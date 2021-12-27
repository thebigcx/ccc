#ifndef STDIO_H
#define STDIO_H

fn extern malloc(uint64) -> *;
fn extern memset(*, uint8, uint64);

fn extern printf(int8*, ...);

#endif