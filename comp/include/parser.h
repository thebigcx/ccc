#pragma once

struct ast;
struct token;

int parse(struct token *toks, struct ast *ast);