" Vim syntax file
" Language:     Lox

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword loxKeyword
    \ and break class else false fun for if lambda nil
    \ or print return super this true var while

syn keyword loxSpecialValue nil

syn match loxNumber '\d\+'
syn match loxNumber '\d\+\.\d*'
syn match loxNumber '-\d\+'
syn match loxNumber '-\d\+\.\d*'

syn region loxString start='"' end='"'

syn keyword loxTodo TODO XXX FIXME NOTE

syn match loxComment "//.*$" contains=loxTodo
syn region loxComment start="/\*" end="\*/" fold extend contains=loxTodo

syn region loxBlock start="{" end="}" transparent fold

hi def link loxKeyword          Keyword
hi def link loxSpecialValue     Constant
hi def link loxString           String
hi def link loxNumber           Number
hi def link loxComment          Comment
hi def link loxTodo             Todo

let b:current_syntax = "lox"

let &cpo = s:cpo_save
unlet s:cpo_save
