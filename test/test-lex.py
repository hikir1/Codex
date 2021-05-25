import unittest
import ply.lex

from context import codex
import codex.lexer

class LexerTestCase(unittest.TestCase):
	def setUp(self):
		self.lexer = ply.lex.lex(module=codex.lexer)
	def assertToksEqual(self, input_text, expected):
		self.lexer.input(input_text)
		tok_cnt = 0
		for tok, exp in zip(self.lexer, expected):
			self.assertEqual(tok.type, exp[0])
			self.assertEqual(tok.value, exp[1])
			tok_cnt += 1
		self.assertEqual(tok_cnt, len(expected), "Expected more toks")
		self.assertFalse(self.lexer.token(), "More toks than expected")

LTC = LexerTestCase

word = "word"
space_exp = [("SPACE", None)]
endpara_exp = [("ENDPARA", None)]
word_exp = [
	("CHAR", "w"),
	("CHAR", "o"),
	("CHAR", "r"),
	("CHAR", "d"),
]

class TestSpace(LTC):
	def test_space(self):
		self.assertToksEqual(" ", space_exp)
		self.assertToksEqual("  ", space_exp)
	def test_newline(self):
		self.assertToksEqual("\n", space_exp)
	def test_space_and_newline(self):
		self.assertToksEqual("\n ", space_exp)
		self.assertToksEqual(" \n", space_exp)
		self.assertToksEqual(" \n ", space_exp)

class TestText(LTC):
	def test_text_word(self):
		self.assertToksEqual(word, word_exp)
	def test_text_ascii_alnum(self):
		ascii_alnum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
		self.assertToksEqual(ascii_alnum, [("CHAR", c) for c in ascii_alnum])
	def test_text_with_space(self):
		self.assertToksEqual(" " + word, space_exp + word_exp)
		self.assertToksEqual(word + " ", word_exp + space_exp)
	def test_text_with_multispace(self):
		self.assertToksEqual("  " + word, space_exp + word_exp)
		self.assertToksEqual(word + "  ", word_exp + space_exp)
	def test_text_multiword(self):
		self.assertToksEqual(word + " " + word, word_exp + space_exp + word_exp)
		self.assertToksEqual(" " + word + " " + word + " ", (space_exp + word_exp)*2 + space_exp)
	def test_text_with_newline(self):
		self.assertToksEqual("\n" + word, space_exp + word_exp)
		self.assertToksEqual(word + "\n", word_exp + space_exp)
	def test_multline_text(self):
		self.assertToksEqual(word + "\n" + word, word_exp + space_exp + word_exp)
		self.assertToksEqual(word + "\n" + word + "\n", (word_exp + space_exp)*2)
		self.assertToksEqual("\n" + word + "\n" + word, (space_exp + word_exp)*2)

class TestEndPara(LTC):
	def test_just_para(self):
		self.assertToksEqual("\n\n", endpara_exp)
	def test_far_para(self):
		self.assertToksEqual("\n\n\n", endpara_exp)
	def test_para_with_space(self):
		self.assertToksEqual(" \n\n", endpara_exp)
		self.assertToksEqual("  \n\n", endpara_exp)
		self.assertToksEqual("\n\n ", endpara_exp)
		self.assertToksEqual("\n\n  ", endpara_exp)
		self.assertToksEqual(" \n\n\n", endpara_exp)
		self.assertToksEqual("  \n\n\n", endpara_exp)
	def test_far_para_space_inbetween(self):
		self.assertToksEqual("\n\n \n\n", endpara_exp)
		self.assertToksEqual("\n\n   \n\n", endpara_exp)
	def test_para_with_text(self):
		self.assertToksEqual(word + "\n\n", word_exp + endpara_exp)
		self.assertToksEqual("\n\n" + word, endpara_exp + word_exp)
		self.assertToksEqual("\n\n" + word, endpara_exp + word_exp)
	def test_multi_para(self):
		self.assertToksEqual("\n\n" + word + "\n\n", endpara_exp + word_exp + endpara_exp)
		self.assertToksEqual( word + "\n\n" + word + "\n\n", (word_exp + endpara_exp)*2)
	def test_para_with_text_and_space(self):
		self.assertToksEqual(word + " \n\n", word_exp + endpara_exp)
		self.assertToksEqual(word + "\n\n ", word_exp + endpara_exp)
		self.assertToksEqual(word + "\n\n \n\n", word_exp + endpara_exp)
		self.assertToksEqual(" " + word + " \n\n", space_exp + word_exp + endpara_exp)
		

"""
class TestComment(LTC):
	def test_only_comment(self):
		self.assertToksEqual(";;comment", [])
		self.assertToksEqual(";; comment", [])
		self.assertToksEqual("\n;; comment\n", [])
	def test_multiple_comments(self):
		self.assertToksEqual(";; comment\n;; comment", [])
	def test_nested_comment(self):
		self.assertToksEqual(";; comment ;; nested", [])
	def test_blank_comment(self):
		self.assertToksEqual(";;", [])
		self.assertToksEqual(";;\n;;", [])

class TestTextMarkup(LTC):
	text = "this is text"
	tags = [
			("**", "strong"),
			("*", "emph"),
			("//", "italic"),
			("__", "under"),
			("--", "strike"),
			("||", "mark"),
	]
	def test_only_tags(self):
		for tag, tag_name in tags:
			self.assertToksEqual(tag + text + tag, 
"""


