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

#include "ast_java.h"

#include "Type.h"
#include "code_writer.h"

namespace android {
namespace aidl {

void
WriteModifiers(CodeWriter* to, int mod, int mask)
{
    int m = mod & mask;

    if (m & OVERRIDE) {
        to->Write("@Override ");
    }

    if ((m & SCOPE_MASK) == PUBLIC) {
        to->Write("public ");
    }
    else if ((m & SCOPE_MASK) == PRIVATE) {
        to->Write("private ");
    }
    else if ((m & SCOPE_MASK) == PROTECTED) {
        to->Write("protected ");
    }

    if (m & STATIC) {
        to->Write("static ");
    }
    
    if (m & FINAL) {
        to->Write("final ");
    }

    if (m & ABSTRACT) {
        to->Write("abstract ");
    }
}

void
WriteArgumentList(CodeWriter* to, const vector<Expression*>& arguments)
{
    size_t N = arguments.size();
    for (size_t i=0; i<N; i++) {
        arguments[i]->Write(to);
        if (i != N-1) {
            to->Write(", ");
        }
    }
}

ClassElement::ClassElement()
{
}

ClassElement::~ClassElement()
{
}

Field::Field()
    :ClassElement(),
     modifiers(0),
     variable(NULL)
{
}

Field::Field(int m, Variable* v)
    :ClassElement(),
     modifiers(m),
     variable(v)
{
}

Field::~Field()
{
}

void
Field::GatherTypes(set<Type*>* types) const
{
    types->insert(this->variable->type);
}

void
Field::Write(CodeWriter* to) const
{
    if (this->comment.length() != 0) {
        to->Write("%s\n", this->comment.c_str());
    }
    WriteModifiers(to, this->modifiers, SCOPE_MASK | STATIC | FINAL | OVERRIDE);
    to->Write("%s %s", this->variable->type->QualifiedName().c_str(),
            this->variable->name.c_str());
    if (this->value.length() != 0) {
        to->Write(" = %s", this->value.c_str());
    }
    to->Write(";\n");
}

Expression::~Expression()
{
}

LiteralExpression::LiteralExpression(const string& v)
    :value(v)
{
}

LiteralExpression::~LiteralExpression()
{
}

void
LiteralExpression::Write(CodeWriter* to) const
{
    to->Write("%s", this->value.c_str());
}

StringLiteralExpression::StringLiteralExpression(const string& v)
    :value(v)
{
}

StringLiteralExpression::~StringLiteralExpression()
{
}

void
StringLiteralExpression::Write(CodeWriter* to) const
{
    to->Write("\"%s\"", this->value.c_str());
}

Variable::Variable()
    :type(NULL),
     name(),
     dimension(0)
{
}

Variable::Variable(Type* t, const string& n)
    :type(t),
     name(n),
     dimension(0)
{
}

Variable::Variable(Type* t, const string& n, int d)
    :type(t),
     name(n),
     dimension(d)
{
}

Variable::~Variable()
{
}

void
Variable::GatherTypes(set<Type*>* types) const
{
    types->insert(this->type);
}

void
Variable::WriteDeclaration(CodeWriter* to) const
{
    string dim;
    for (int i=0; i<this->dimension; i++) {
        dim += "[]";
    }
    to->Write("%s%s %s", this->type->QualifiedName().c_str(), dim.c_str(),
            this->name.c_str());
}

void
Variable::Write(CodeWriter* to) const
{
    to->Write("%s", name.c_str());
}

FieldVariable::FieldVariable(Expression* o, const string& n)
    :object(o),
     clazz(NULL),
     name(n)
{
}

FieldVariable::FieldVariable(Type* c, const string& n)
    :object(NULL),
     clazz(c),
     name(n)
{
}

FieldVariable::~FieldVariable()
{
}

void
FieldVariable::Write(CodeWriter* to) const
{
    if (this->object != NULL) {
        this->object->Write(to);
    }
    else if (this->clazz != NULL) {
        to->Write("%s", this->clazz->QualifiedName().c_str());
    }
    to->Write(".%s", name.c_str());
}


Statement::~Statement()
{
}

StatementBlock::StatementBlock()
{
}

StatementBlock::~StatementBlock()
{
}

void
StatementBlock::Write(CodeWriter* to) const
{
    to->Write("{\n");
    int N = this->statements.size();
    for (int i=0; i<N; i++) {
        this->statements[i]->Write(to);
    }
    to->Write("}\n");
}

void
StatementBlock::Add(Statement* statement)
{
    this->statements.push_back(statement);
}

void
StatementBlock::Add(Expression* expression)
{
    this->statements.push_back(new ExpressionStatement(expression));
}

ExpressionStatement::ExpressionStatement(Expression* e)
    :expression(e)
{
}

ExpressionStatement::~ExpressionStatement()
{
}

void
ExpressionStatement::Write(CodeWriter* to) const
{
    this->expression->Write(to);
    to->Write(";\n");
}

Assignment::Assignment(Variable* l, Expression* r)
    :lvalue(l),
     rvalue(r),
     cast(NULL)
{
}

Assignment::Assignment(Variable* l, Expression* r, Type* c)
    :lvalue(l),
     rvalue(r),
     cast(c)
{
}

Assignment::~Assignment()
{
}

void
Assignment::Write(CodeWriter* to) const
{
    this->lvalue->Write(to);
    to->Write(" = ");
    if (this->cast != NULL) {
        to->Write("(%s)", this->cast->QualifiedName().c_str());
    }
    this->rvalue->Write(to);
}

MethodCall::MethodCall(const string& n)
    :obj(NULL),
     clazz(NULL),
     name(n)
{
}

MethodCall::MethodCall(const string& n, int argc = 0, ...)
    :obj(NULL),
     clazz(NULL),
     name(n)
{
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

MethodCall::MethodCall(Expression* o, const string& n)
    :obj(o),
     clazz(NULL),
     name(n)
{
}

MethodCall::MethodCall(Type* t, const string& n)
    :obj(NULL),
     clazz(t),
     name(n)
{
}

MethodCall::MethodCall(Expression* o, const string& n, int argc = 0, ...)
    :obj(o),
     clazz(NULL),
     name(n)
{
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

MethodCall::MethodCall(Type* t, const string& n, int argc = 0, ...)
    :obj(NULL),
     clazz(t),
     name(n)
{
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

MethodCall::~MethodCall()
{
}

void
MethodCall::init(int n, va_list args)
{
    for (int i=0; i<n; i++) {
        Expression* expression = (Expression*)va_arg(args, void*);
        this->arguments.push_back(expression);
    }
}

void
MethodCall::Write(CodeWriter* to) const
{
    if (this->obj != NULL) {
        this->obj->Write(to);
        to->Write(".");
    }
    else if (this->clazz != NULL) {
        to->Write("%s.", this->clazz->QualifiedName().c_str());
    }
    to->Write("%s(", this->name.c_str());
    WriteArgumentList(to, this->arguments);
    to->Write(")");
}

Comparison::Comparison(Expression* l, const string& o, Expression* r)
    :lvalue(l),
     op(o),
     rvalue(r)
{
}

Comparison::~Comparison()
{
}

void
Comparison::Write(CodeWriter* to) const
{
    to->Write("(");
    this->lvalue->Write(to);
    to->Write("%s", this->op.c_str());
    this->rvalue->Write(to);
    to->Write(")");
}

NewExpression::NewExpression(Type* t)
    :type(t)
{
}

NewExpression::NewExpression(Type* t, int argc = 0, ...)
    :type(t)
{
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

NewExpression::~NewExpression()
{
}

void
NewExpression::init(int n, va_list args)
{
    for (int i=0; i<n; i++) {
        Expression* expression = (Expression*)va_arg(args, void*);
        this->arguments.push_back(expression);
    }
}

void
NewExpression::Write(CodeWriter* to) const
{
    to->Write("new %s(", this->type->InstantiableName().c_str());
    WriteArgumentList(to, this->arguments);
    to->Write(")");
}

NewArrayExpression::NewArrayExpression(Type* t, Expression* s)
    :type(t),
     size(s)
{
}

NewArrayExpression::~NewArrayExpression()
{
}

void
NewArrayExpression::Write(CodeWriter* to) const
{
    to->Write("new %s[", this->type->QualifiedName().c_str());
    size->Write(to);
    to->Write("]");
}

Ternary::Ternary()
    :condition(NULL),
     ifpart(NULL),
     elsepart(NULL)
{
}

Ternary::Ternary(Expression* a, Expression* b, Expression* c)
    :condition(a),
     ifpart(b),
     elsepart(c)
{
}

Ternary::~Ternary()
{
}

void
Ternary::Write(CodeWriter* to) const
{
    to->Write("((");
    this->condition->Write(to);
    to->Write(")?(");
    this->ifpart->Write(to);
    to->Write("):(");
    this->elsepart->Write(to);
    to->Write("))");
}

Cast::Cast()
    :type(NULL),
     expression(NULL)
{
}

Cast::Cast(Type* t, Expression* e)
    :type(t),
     expression(e)
{
}

Cast::~Cast()
{
}

void
Cast::Write(CodeWriter* to) const
{
    to->Write("((%s)", this->type->QualifiedName().c_str());
    expression->Write(to);
    to->Write(")");
}

VariableDeclaration::VariableDeclaration(Variable* l, Expression* r, Type* c)
    :lvalue(l),
     cast(c),
     rvalue(r)
{
}

VariableDeclaration::VariableDeclaration(Variable* l)
    :lvalue(l),
     cast(NULL),
     rvalue(NULL)
{
}

VariableDeclaration::~VariableDeclaration()
{
}

void
VariableDeclaration::Write(CodeWriter* to) const
{
    this->lvalue->WriteDeclaration(to);
    if (this->rvalue != NULL) {
        to->Write(" = ");
        if (this->cast != NULL) {
            to->Write("(%s)", this->cast->QualifiedName().c_str());
        }
        this->rvalue->Write(to);
    }
    to->Write(";\n");
}

IfStatement::IfStatement()
    :expression(NULL),
     statements(new StatementBlock),
     elseif(NULL)
{
}

IfStatement::~IfStatement()
{
}

void
IfStatement::Write(CodeWriter* to) const
{
    if (this->expression != NULL) {
        to->Write("if (");
        this->expression->Write(to);
        to->Write(") ");
    }
    this->statements->Write(to);
    if (this->elseif != NULL) {
        to->Write("else ");
        this->elseif->Write(to);
    }
}

ReturnStatement::ReturnStatement(Expression* e)
    :expression(e)
{
}

ReturnStatement::~ReturnStatement()
{
}

void
ReturnStatement::Write(CodeWriter* to) const
{
    to->Write("return ");
    this->expression->Write(to);
    to->Write(";\n");
}

TryStatement::TryStatement()
    :statements(new StatementBlock)
{
}

TryStatement::~TryStatement()
{
}

void
TryStatement::Write(CodeWriter* to) const
{
    to->Write("try ");
    this->statements->Write(to);
}

CatchStatement::CatchStatement(Variable* e)
    :statements(new StatementBlock),
     exception(e)
{
}

CatchStatement::~CatchStatement()
{
}

void
CatchStatement::Write(CodeWriter* to) const
{
    to->Write("catch ");
    if (this->exception != NULL) {
        to->Write("(");
        this->exception->WriteDeclaration(to);
        to->Write(") ");
    }
    this->statements->Write(to);
}

FinallyStatement::FinallyStatement()
    :statements(new StatementBlock)
{
}

FinallyStatement::~FinallyStatement()
{
}

void
FinallyStatement::Write(CodeWriter* to) const
{
    to->Write("finally ");
    this->statements->Write(to);
}

Case::Case()
    :statements(new StatementBlock)
{
}

Case::Case(const string& c)
    :statements(new StatementBlock)
{
    cases.push_back(c);
}

Case::~Case()
{
}

void
Case::Write(CodeWriter* to) const
{
    int N = this->cases.size();
    if (N > 0) {
        for (int i=0; i<N; i++) {
            string s = this->cases[i];
            if (s.length() != 0) {
                to->Write("case %s:\n", s.c_str());
            } else {
                to->Write("default:\n");
            }
        }
    } else {
        to->Write("default:\n");
    }
    statements->Write(to);
}

SwitchStatement::SwitchStatement(Expression* e)
    :expression(e)
{
}

SwitchStatement::~SwitchStatement()
{
}

void
SwitchStatement::Write(CodeWriter* to) const
{
    to->Write("switch (");
    this->expression->Write(to);
    to->Write(")\n{\n");
    int N = this->cases.size();
    for (int i=0; i<N; i++) {
        this->cases[i]->Write(to);
    }
    to->Write("}\n");
}

Break::Break()
{
}

Break::~Break()
{
}

void
Break::Write(CodeWriter* to) const
{
    to->Write("break;\n");
}

Method::Method()
    :ClassElement(),
     modifiers(0),
     returnType(NULL), // (NULL means constructor)
     returnTypeDimension(0),
     statements(NULL)
{
}

Method::~Method()
{
}

void
Method::GatherTypes(set<Type*>* types) const
{
    size_t N, i;

    if (this->returnType) {
        types->insert(this->returnType);
    }

    N = this->parameters.size();
    for (i=0; i<N; i++) {
        this->parameters[i]->GatherTypes(types);
    }

    N = this->exceptions.size();
    for (i=0; i<N; i++) {
        types->insert(this->exceptions[i]);
    }
}

void
Method::Write(CodeWriter* to) const
{
    size_t N, i;

    if (this->comment.length() != 0) {
        to->Write("%s\n", this->comment.c_str());
    }

    WriteModifiers(to, this->modifiers, SCOPE_MASK | STATIC | ABSTRACT | FINAL | OVERRIDE);

    if (this->returnType != NULL) {
        string dim;
        for (i=0; i<this->returnTypeDimension; i++) {
            dim += "[]";
        }
        to->Write("%s%s ", this->returnType->QualifiedName().c_str(),
                dim.c_str());
    }
   
    to->Write("%s(", this->name.c_str());

    N = this->parameters.size();
    for (i=0; i<N; i++) {
        this->parameters[i]->WriteDeclaration(to);
        if (i != N-1) {
            to->Write(", ");
        }
    }

    to->Write(")");

    N = this->exceptions.size();
    for (i=0; i<N; i++) {
        if (i == 0) {
            to->Write(" throws ");
        } else {
            to->Write(", ");
        }
        to->Write("%s", this->exceptions[i]->QualifiedName().c_str());
    }

    if (this->statements == NULL) {
        to->Write(";\n");
    } else {
        to->Write("\n");
        this->statements->Write(to);
    }
}

Class::Class()
    :modifiers(0),
     what(CLASS),
     type(NULL),
     extends(NULL)
{
}

Class::~Class()
{
}

void
Class::GatherTypes(set<Type*>* types) const
{
    int N, i;

    types->insert(this->type);
    if (this->extends != NULL) {
        types->insert(this->extends);
    }

    N = this->interfaces.size();
    for (i=0; i<N; i++) {
        types->insert(this->interfaces[i]);
    }

    N = this->elements.size();
    for (i=0; i<N; i++) {
        this->elements[i]->GatherTypes(types);
    }
}

void
Class::Write(CodeWriter* to) const
{
    size_t N, i;

    if (this->comment.length() != 0) {
        to->Write("%s\n", this->comment.c_str());
    }

    WriteModifiers(to, this->modifiers, ALL_MODIFIERS);

    if (this->what == Class::CLASS) {
        to->Write("class ");
    } else {
        to->Write("interface ");
    }

    string name = this->type->Name();
    size_t pos = name.rfind('.');
    if (pos != string::npos) {
        name = name.c_str() + pos + 1;
    }

    to->Write("%s", name.c_str());

    if (this->extends != NULL) {
        to->Write(" extends %s", this->extends->QualifiedName().c_str());
    }

    N = this->interfaces.size();
    if (N != 0) {
        if (this->what == Class::CLASS) {
            to->Write(" implements");
        } else {
            to->Write(" extends");
        }
        for (i=0; i<N; i++) {
            to->Write(" %s", this->interfaces[i]->QualifiedName().c_str());
        }
    }

    to->Write("\n");
    to->Write("{\n");

    N = this->elements.size();
    for (i=0; i<N; i++) {
        this->elements[i]->Write(to);
    }

    to->Write("}\n");

}

Document::Document()
{
}

Document::~Document()
{
}

static string
escape_backslashes(const string& str)
{
    string result;
    const size_t I=str.length();
    for (size_t i=0; i<I; i++) {
        char c = str[i];
        if (c == '\\') {
            result += "\\\\";
        } else {
            result += c;
        }
    }
    return result;
}

void
Document::Write(CodeWriter* to) const
{
    size_t N, i;

    if (this->comment.length() != 0) {
        to->Write("%s\n", this->comment.c_str());
    }
    to->Write("/*\n"
                " * This file is auto-generated.  DO NOT MODIFY.\n"
                " * Original file: %s\n"
                " */\n", escape_backslashes(this->originalSrc).c_str());
    if (this->package.length() != 0) {
        to->Write("package %s;\n", this->package.c_str());
    }

    N = this->classes.size();
    for (i=0; i<N; i++) {
        Class* c = this->classes[i];
        c->Write(to);
    }
}

}  // namespace aidl
}  // namespace android