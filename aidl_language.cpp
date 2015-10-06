#include "aidl_language.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "aidl_language_y.hpp"
#include "parse_helpers.h"

#ifdef _WIN32
int isatty(int  fd)
{
    return (fd == 0);
}
#endif

using std::string;
using std::cerr;
using std::endl;
using android::aidl::cpp_strdup;

void yylex_init(void **);
void yylex_destroy(void *);
void yyset_in(FILE *f, void *);
int yyparse(Parser*);
YY_BUFFER_STATE yy_scan_string(const char *, void *);
void yy_delete_buffer(YY_BUFFER_STATE, void *);

Parser::Parser(const string& filename)
    : filename_(filename) {
  yylex_init(&scanner_);
}

Parser::~Parser() {
  if (buffer_is_valid_)
    yy_delete_buffer(buffer_, scanner_);
  yylex_destroy(scanner_);
}

AidlType::AidlType(const std::string& name, unsigned line,
                   const std::string& comments, bool is_array)
    : name_(name),
      line_(line),
      is_array_(is_array),
      comments_(comments) {}

string AidlType::ToString() const {
  return name_ + (is_array_ ? "[]" : "");
}

AidlArgument::AidlArgument(AidlArgument::Direction direction, AidlType* type,
                           std::string name, unsigned line)
    : type_(type),
      direction_(direction),
      direction_specified_(true),
      name_(name),
      line_(line) {}

AidlArgument::AidlArgument(AidlType* type, std::string name, unsigned line)
    : type_(type),
      direction_(AidlArgument::IN_DIR),
      direction_specified_(false),
      name_(name),
      line_(line) {}

string AidlArgument::ToString() const {
  string ret;

  if (direction_specified_) {
    switch(direction_) {
    case AidlArgument::IN_DIR:
      ret += "in ";
      break;
    case AidlArgument::OUT_DIR:
      ret += "out ";
      break;
    case AidlArgument::INOUT_DIR:
      ret += "inout ";
      break;
    }
  }

  ret += type_->ToString();
  ret += " ";
  ret += name_;

  return ret;
}

AidlMethod::AidlMethod(bool oneway, AidlType* type, std::string name,
                       std::vector<std::unique_ptr<AidlArgument>>* args,
                       unsigned line, std::string comments, int id)
    : oneway_(oneway),
      comments_(comments),
      type_(type),
      name_(name),
      line_(line),
      arguments_(std::move(*args)),
      id_(id) {
  has_id_ = true;
  delete args;
}

AidlMethod::AidlMethod(bool oneway, AidlType* type, std::string name,
                       std::vector<std::unique_ptr<AidlArgument>>* args,
                       unsigned line, std::string comments)
    : AidlMethod(oneway, type, name, args, line, comments, 0) {
  has_id_ = false;
}

void Parser::ReportError(const string& err) {
  /* FIXME: We're printing out the line number as -1. We used to use yylineno
   * (which was NEVER correct even before reentrant parsing). Now we'll need
   * another way.
   */
  cerr << filename_ << ":" << -1 << ": " << err << endl;
  error_ = 1;
}

bool Parser::OpenFileFromDisk() {
  FILE *in = fopen(FileName().c_str(), "r");

  if (! in)
    return false;

  yyset_in(in, Scanner());
  return true;
}

void Parser::SetFileContents(const std::string& contents) {
  if (buffer_is_valid_)
    yy_delete_buffer(buffer_, scanner_);

  buffer_ = yy_scan_string(contents.c_str(), scanner_);
  buffer_is_valid_ = true;
}

bool Parser::RunParser() {
  int ret = yy::parser(this).parse();

  return ret == 0 && error_ == 0;
}

void Parser::AddImport(std::vector<std::string>* terms, unsigned line) {
  std::string data;
  bool first = true;

  /* NOTE: This string building code is duplicated from below. We haven't
   * factored it out into a function because it's hoped that when import_info
   * becomes a class we won't need this anymore.
   **/
  for (const auto& term : *terms) {
      if (first)
          data = term;
      else
          data += '.' + term;
  }

  import_info* import = new import_info();
  memset(import, 0, sizeof(import_info));
  import->from = cpp_strdup(this->FileName().c_str());
  import->next = imports_;
  import->line = line;
  import->neededClass = cpp_strdup(data.c_str());
  imports_ = import;

  delete terms;
}

void Parser::SetPackage(std::vector<std::string> *terms) {
    bool first = true;

    for (const auto& term : *terms) {
        if (first)
            package_ = term;
        else
            package_ += '.' + term;
    }

    delete terms;
}
