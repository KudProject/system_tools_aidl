/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "options.h"
#include "logging.h"
#include "os.h"

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include <android-base/strings.h>

using android::base::Split;
using android::base::Trim;
using std::endl;
using std::string;

namespace android {
namespace aidl {

string Options::GetUsage() const {
  std::ostringstream sstr;
  sstr << "usage:" << endl
       << myname_ << " --lang={java|cpp} [OPTION]... INPUT..." << endl
       << "   Generate Java or C++ files for AIDL file(s)." << endl
       << endl
       << myname_ << " --preprocess OUTPUT INPUT..." << endl
       << "   Create an AIDL file having declarations of AIDL file(s)." << endl
       << endl
#ifndef _WIN32
       << myname_ << " --dumpapi --out=DIR INPUT..." << endl
       << "   Dump API signature of AIDL file(s) to DIR." << endl
       << endl
       << myname_ << " --checkapi OLD_DIR NEW_DIR" << endl
       << "   Checkes whether API dump NEW_DIR is backwards compatible extension " << endl
       << "   of the API dump OLD_DIR." << endl
#endif
       << endl;

  // Legacy option formats
  if (language_ == Options::Language::JAVA) {
    sstr << myname_ << " [OPTION]... INPUT [OUTPUT]" << endl
         << "   Generate a Java file for an AIDL file." << endl
         << endl;
  } else if (language_ == Options::Language::CPP) {
    sstr << myname_ << " [OPTION]... INPUT HEADER_DIR OUTPUT" << endl
         << "   Generate C++ headers and source for an AIDL file." << endl
         << endl;
  }

  sstr << "OPTION:" << endl
       << "  -I DIR, --include=DIR" << endl
       << "          Use DIR as a search path for import statements." << endl
       << "  -m FILE, --import=FILE" << endl
       << "          Import FILE directly without searching in the search paths." << endl
       << "  -p FILE, --preprocessed=FILE" << endl
       << "          Include FILE which is created by --preprocess." << endl
       << "  -d FILE, --dep=FILE" << endl
       << "          Generate dependency file as FILE. Don't use this when" << endl
       << "          there are multiple input files. Use -a then." << endl
       << "  -o DIR, --out=DIR" << endl
       << "          Use DIR as the base output directory for generated files." << endl
       << "  -h DIR, --header_out=DIR" << endl
       << "          Generate C++ headers under DIR." << endl
       << "  -a" << endl
       << "          Generate dependency file next to the output file with the" << endl
       << "          name based on the input file." << endl
       << "  -b" << endl
       << "          Trigger fail when trying to compile a parcelable." << endl
       << "  --ninja" << endl
       << "          Generate dependency file in a format ninja understands." << endl
       << "  --structured" << endl
       << "          Whether this interface is defined exclusively in AIDL." << endl
       << "          It is therefore a candidate for stabilization." << endl
       << "  -t, --trace" << endl
       << "          Include tracing code for systrace. Note that if either" << endl
       << "          the client or service code is not auto-generated by this" << endl
       << "          tool, that part will not be traced." << endl
       << "  --transaction_names" << endl
       << "          Generate transaction names." << endl
       << "  --apimapping" << endl
       << "          Generates a mapping of declared aidl method signatures to" << endl
       << "          the original line number. e.g.: " << endl
       << "              If line 39 of foo/bar/IFoo.aidl contains:"
       << "              void doFoo(int bar, String baz);" << endl
       << "              Then the result would be:" << endl
       << "              foo.bar.Baz|doFoo|int,String,|void" << endl
       << "              foo/bar/IFoo.aidl:39" << endl
       << "  -v VER, --version=VER" << endl
       << "          Set the version of the interface and parcelable to VER." << endl
       << "          VER must be an interger greater than 0." << endl
       << "  --log" << endl
       << "          Information about the transaction, e.g., method name, argument" << endl
       << "          values, execution time, etc., is provided via callback." << endl
       << "  --help" << endl
       << "          Show this help." << endl
       << endl
       << "INPUT:" << endl
       << "  An AIDL file." << endl
       << endl
       << "OUTPUT:" << endl
       << "  Path to the generated Java or C++ source file. This is ignored when" << endl
       << "  -o or --out is specified or the number of the input files are" << endl
       << "  more than one." << endl
       << "  For Java, if omitted, Java source file is generated at the same" << endl
       << "  place as the input AIDL file," << endl
       << endl
       << "HEADER_DIR:" << endl
       << "  Path to where C++ headers are generated." << endl;
  return sstr.str();
}

Options Options::From(const string& cmdline) {
  vector<string> args = Split(cmdline, " ");
  return From(args);
}

Options Options::From(const vector<string>& args) {
  Options::Language lang = Options::Language::JAVA;
  int argc = args.size();
  if (argc >= 1 && args.at(0) == "aidl-cpp") {
    lang = Options::Language::CPP;
  }
  const char* argv[argc + 1];
  for (int i = 0; i < argc; i++) {
    argv[i] = args.at(i).c_str();
  }
  argv[argc] = nullptr;

  return Options(argc, argv, lang);
}

Options::Options(int argc, const char* const argv[], Options::Language default_lang)
    : myname_(argv[0]), language_(default_lang) {
  bool lang_option_found = false;
  optind = 0;
  while (true) {
    static struct option long_options[] = {
        {"lang", required_argument, 0, 'l'},
        {"preprocess", no_argument, 0, 's'},
#ifndef _WIN32
        {"dumpapi", no_argument, 0, 'u'},
        {"checkapi", no_argument, 0, 'A'},
#endif
        {"apimapping", required_argument, 0, 'i'},
        {"include", required_argument, 0, 'I'},
        {"import", required_argument, 0, 'm'},
        {"preprocessed", required_argument, 0, 'p'},
        {"dep", required_argument, 0, 'd'},
        {"out", required_argument, 0, 'o'},
        {"header_out", required_argument, 0, 'h'},
        {"ninja", no_argument, 0, 'n'},
        {"structured", no_argument, 0, 'S'},
        {"trace", no_argument, 0, 't'},
        {"transaction_names", no_argument, 0, 'c'},
        {"version", required_argument, 0, 'v'},
        {"log", no_argument, 0, 'L'},
        {"help", no_argument, 0, 'e'},
        {0, 0, 0, 0},
    };
    const int c = getopt_long(argc, const_cast<char* const*>(argv),
                              "I:m:p:d:o:h:abtv:", long_options, nullptr);
    if (c == -1) {
      // no more options
      break;
    }
    switch (c) {
      case 'l':
        if (language_ == Options::Language::CPP) {
          // aidl-cpp can't set language. aidl-cpp exists only for backwards
          // compatibility.
          error_message_ << "aidl-cpp does not support --lang." << endl;
          return;
        } else {
          lang_option_found = true;
          string lang = Trim(optarg);
          if (lang == "java") {
            language_ = Options::Language::JAVA;
            task_ = Options::Task::COMPILE;
          } else if (lang == "cpp") {
            language_ = Options::Language::CPP;
            task_ = Options::Task::COMPILE;
          } else if (lang == "ndk") {
            language_ = Options::Language::NDK;
            task_ = Options::Task::COMPILE;
          } else {
            error_message_ << "Unsupported language: '" << lang << "'" << endl;
            return;
          }
        }
        break;
      case 's':
        if (task_ != Options::Task::UNSPECIFIED) {
          task_ = Options::Task::PREPROCESS;
        }
        break;
#ifndef _WIN32
      case 'u':
        if (task_ != Options::Task::UNSPECIFIED) {
          task_ = Options::Task::DUMP_API;
        }
        break;
      case 'A':
        if (task_ != Options::Task::UNSPECIFIED) {
          task_ = Options::Task::CHECK_API;
          // to ensure that all parcelables in the api dumpes are structured
          structured_ = true;
        }
        break;
#endif
      case 'I': {
        import_dirs_.emplace(Trim(optarg));
        break;
      }
      case 'm': {
        import_files_.emplace(Trim(optarg));
        break;
      }
      case 'p':
        preprocessed_files_.emplace_back(Trim(optarg));
        break;
      case 'd':
        dependency_file_ = Trim(optarg);
        break;
      case 'o':
        output_dir_ = Trim(optarg);
        if (output_dir_.back() != OS_PATH_SEPARATOR) {
          output_dir_.push_back(OS_PATH_SEPARATOR);
        }
        break;
      case 'h':
        output_header_dir_ = Trim(optarg);
        if (output_header_dir_.back() != OS_PATH_SEPARATOR) {
          output_header_dir_.push_back(OS_PATH_SEPARATOR);
        }
        break;
      case 'n':
        dependency_file_ninja_ = true;
        break;
      case 'S':
        structured_ = true;
        break;
      case 't':
        gen_traces_ = true;
        break;
      case 'a':
        auto_dep_file_ = true;
        break;
      case 'b':
        fail_on_parcelable_ = true;
        break;
      case 'c':
        gen_transaction_names_ = true;
        break;
      case 'v': {
        const string ver_str = Trim(optarg);
        int ver = atoi(ver_str.c_str());
        if (ver > 0) {
          version_ = ver;
        } else {
          error_message_ << "Invalid version number: '" << ver_str << "'. "
                         << "Version must be a positive natural number." << endl;
          return;
        }
        break;
      }
      case 'L':
        gen_log_ = true;
        break;
      case 'e':
        std::cerr << GetUsage();
        exit(0);
      case 'i':
        output_file_ = Trim(optarg);
        task_ = Task::DUMP_MAPPINGS;
        break;
      default:
        std::cerr << GetUsage();
        exit(1);
    }
  }  // while

  // Positional arguments
  if (!lang_option_found && task_ == Options::Task::COMPILE) {
    // the legacy arguments format
    if (argc - optind <= 0) {
      error_message_ << "No input file" << endl;
      return;
    }
    if (language_ == Options::Language::JAVA) {
      input_files_.emplace_back(argv[optind++]);
      if (argc - optind >= 1) {
        output_file_ = argv[optind++];
      } else if (output_dir_.empty()) {
        // when output is omitted and -o option isn't set, the output is by
        // default set to the input file path with .aidl is replaced to .java.
        // If -o option is set, the output path is calculated by
        // generate_outputFileName which returns "<output_dir>/<package/name>/
        // <typename>.java"
        output_file_ = input_files_.front();
        if (android::base::EndsWith(output_file_, ".aidl")) {
          output_file_ = output_file_.substr(0, output_file_.length() - strlen(".aidl"));
        }
        output_file_ += ".java";
      }
    } else if (IsCppOutput()) {
      input_files_.emplace_back(argv[optind++]);
      if (argc - optind < 2) {
        error_message_ << "No HEADER_DIR or OUTPUT." << endl;
        return;
      }
      output_header_dir_ = argv[optind++];
      if (output_header_dir_.back() != OS_PATH_SEPARATOR) {
        output_header_dir_.push_back(OS_PATH_SEPARATOR);
      }
      output_file_ = argv[optind++];
    }
    if (argc - optind > 0) {
      error_message_ << "Too many arguments: ";
      for (int i = optind; i < argc; i++) {
        error_message_ << " " << argv[i];
      }
      error_message_ << endl;
    }
  } else {
    // the new arguments format
    if (task_ == Options::Task::COMPILE || task_ == Options::Task::DUMP_API) {
      if (argc - optind < 1) {
        error_message_ << "No input file." << endl;
        return;
      }
    } else {
      if (argc - optind < 2) {
        error_message_ << "Insufficient arguments. At least 2 required, but "
                       << "got " << (argc - optind) << "." << endl;
        return;
      }
      if (task_ != Options::Task::CHECK_API && task_ != Options::Task::DUMP_MAPPINGS) {
        output_file_ = argv[optind++];
      }
    }
    while (optind < argc) {
      input_files_.emplace_back(argv[optind++]);
    }
  }

  // filter out invalid combinations
  if (lang_option_found) {
    if (IsCppOutput() && task_ == Options::Task::COMPILE) {
      if (output_dir_.empty()) {
        error_message_ << "Output directory is not set. Set with --out." << endl;
        return;
      }
      if (output_header_dir_.empty()) {
        error_message_ << "Header output directory is not set. Set with "
                       << "--header_out." << endl;
        return;
      }
    }
    if (language_ == Options::Language::JAVA && task_ == Options::Task::COMPILE) {
      if (output_dir_.empty()) {
        error_message_ << "Output directory is not set. Set with --out." << endl;
        return;
      }
      if (!output_header_dir_.empty()) {
        error_message_ << "Header output directory is set, which does not make "
                       << "sense for Java." << endl;
        return;
      }
    }
  }
  if (task_ == Options::Task::COMPILE) {
    for (const string& input : input_files_) {
      if (!android::base::EndsWith(input, ".aidl")) {
        error_message_ << "Expected .aidl file for input but got '" << input << "'" << endl;
        return;
      }
    }
    if (!output_file_.empty() && input_files_.size() > 1) {
      error_message_ << "Multiple AIDL files can't be compiled to a single "
                     << "output file '" << output_file_ << "'. "
                     << "Use --out=DIR instead for output files." << endl;
      return;
    }
    if (!dependency_file_.empty() && input_files_.size() > 1) {
      error_message_ << "-d or --dep doesn't work when compiling multiple AIDL "
                     << "files. Use '-a' to generate dependency file next to "
                     << "the output file with the name based on the input "
                     << "file." << endl;
      return;
    }
    if (gen_log_ && (language_ != Options::Language::CPP && language_ != Options::Language::NDK)) {
      error_message_ << "--log is currently supported for either --lang=cpp or --lang=ndk" << endl;
      return;
    }
  }
  if (task_ == Options::Task::PREPROCESS) {
    if (version_ > 0) {
      error_message_ << "--version should not be used with '--preprocess'." << endl;
      return;
    }
  }
  if (task_ == Options::Task::CHECK_API) {
    if (input_files_.size() != 2) {
      error_message_ << "--checkapi requires two inputs for comparing, "
                     << "but got " << input_files_.size() << "." << endl;
      return;
    }
  }
  if (task_ == Options::Task::DUMP_API) {
    if (output_dir_.empty()) {
      error_message_ << "--dump_api requires output directory. Use --out." << endl;
      return;
    }
  }

  CHECK(output_dir_.empty() || output_dir_.back() == OS_PATH_SEPARATOR);
  CHECK(output_header_dir_.empty() || output_header_dir_.back() == OS_PATH_SEPARATOR);
}

}  // namespace android
}  // namespace aidl
