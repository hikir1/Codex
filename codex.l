

	struct tok_stream toks;
	struct text_stream texts;

%s PAR
%s WORD
%s POST_WORD
%s COMM

%%

<PAR>__				tok_stream_push(&toks, BEGIN_UNDERLINE);
<WORD,POST_WORD>__	tok_stream_push(&toks, END_UNDERLINE); BEGIN(POST_WORD);
[:alnum:]([[:alnum:]
