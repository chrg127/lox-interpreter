" Vim syntax file
" Language:     Lox

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword loxKeyword
    \ break class else false fun for if lambda nil
    \ print return super this true var while

syn match loxOperator "\(<<\|>>\|[-+*/%&^|<>!=]\)="
syn match loxOperator "[\[\]?:.!~*&%<>^|=,+-]"
syn match loxOperator "/[^/*=]"me=e-1
syn match loxOperator "/$"
syn keyword loxOperator and or operator

syn keyword loxInclude include

syn match loxDelimiter "[();\\]"
syn match loxBraces display "[{}]"

syn keyword loxSpecialValue nil

syn match loxNumbers display transparent "\<\d\|\.\d" contains=loxNumber,loxFloat
" syn match loxNumber '\d\+'
" syn match loxNumber '\d\+\.\d*'
" syn match loxNumber '-\d\+'
" syn match loxNumber '-\d\+\.\d*'

syn match loxNumber display contained "\d\+\(u\=l\{0,2}\|ll\=u\)\>"
" hex number
" syn match loxNumber display contained "0x\x\+\(u\=l\{0,2}\|ll\=u\)\>"
" float
syn match loxFloat display contained "\d\+"
"floating point number, with dot, optional exponent
syn match loxFloat display contained "\d\+\.\d*\(e[-+]\=\d\+\)\="
"floating point number, starting with a dot, optional exponent
" syn match loxFloat display contained "\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, without dot, with exponent
" syn match loxFloat display contained "\d\+e[-+]\=\d\+[fl]\=\>"

syn region loxString start='"' end='"'

syn keyword loxTodo TODO XXX FIXME NOTE BUG

syn match loxComment "//.*$" contains=loxTodo
syn region loxComment start="/\*" end="\*/" fold extend contains=loxTodo

syn region loxBlock start="{" end="}" transparent fold

syn match loxFunction "\<\h\w*\>\(\s\|\n\)*("me=e-1

hi def link loxKeyword          Keyword
hi def link loxSpecialValue     Constant
hi def link loxString           String
hi def link loxNumber           Number
hi def link loxFloat            Number
hi def link loxComment          Comment
hi def link loxTodo             Todo
hi def link loxFunction         Function
hi def link loxOperator         Operator
hi def link loxDelimiter        Delimiter
hi def link loxBraces           Delimiter
hi def link loxInclude          PreProc

let b:current_syntax = "lox"

let &cpo = s:cpo_save
unlet s:cpo_save
