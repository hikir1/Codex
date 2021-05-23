import unittest
import ply.lex
from .context import codex.lexer

class LexerTestCase(unitest.TestCase):
	def setUp(self):
		self.lexer = ply.lex.lex(module=codex.lexer)
	def assertToksEqual(self, in, expected):
		self.lexer.input(in)
		tok_cnt = 0
		for tok, exp in zip(lexer, expected):
			self.assertEqual(tok.type, exp.type)
			self.assertEqual(tok.value, exp.value)
			tok_cnt += 1
		self.assertEqual(tok_cnt, len(expected))

LTC = LexerTestCase

class TestSpace(LTC):
	def test_space(self):
		self.assertToksEqual(" ", [])
		self.assertToksEqual("  ", [])
	def test_newline(self):
		self.assertToksEqual("\n", [])
		self.assertToksEqual("\n\n", [])
	def test_space_and_newline(self):
		self.assertToksEqual("\n ", [])
		self.assertToksEqual(" \n", [])
		self.assertToksEqual(" \n ", [])
		self.assertToksEqual("\n \n", [])
		self.assertToksEqual("\n \n\n ", [])
		self.assertToksEqual(" \n  \n ", [])

class TestText(LTC):
	word = "word"
	eow = [("ENDWORD", "")]
	wordexp = [
		("CHAR", "w"),
		("CHAR", "o"),
		("CHAR", "r"),
		("CHAR", "d"),
		eow,
	]
	def test_text_word(self):
		self.assertToksEqual(word, wordexp + eow)
	def test_text_ascii_alnum(self):
		ascii_alnum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
		self.assertToksEqual(ascii_alnum, [("CHAR", c) for c in ascii_alnum] + eow)
	def test_text_with_space(self):
		self.assertToksEqual(" " + word, wordexp)
		self.assertToksEqual(word + " ", wordexp)
	def test_text_with_multispace(self):
		self.assertToksEqual("  " + word, wordexp)
		self.assertToksEqual(word + "  ", wordexp)
	def test_text_multiword(self):
		self.assertToksEqual(word + " " + word, wordexp*2)
		self.assertToksEqual(" " + word + " " + word + " ", wordexp*2)
	def test_text_with_newline(self):
		self.assertToksEqual("\n" + word, wordexp)
		self.assertToksEqual("\n\n" + word, wordexp)
		self.assertToksEqual(word + "\n", wordexp)
	def test_text_with_multiline(self):
		self.assertToksEqual("\n\n" + word, wordexp)
		self.assertToksEqual(word + "\n\n", wordexp)
	def test_multline_text(slf):
		self.assertToksEqual(word + "\n" + word, wordexp*2)
		self.assertToksEqual(word + "\n" + word + "\n", wordexp*2)
		self.assertToksEqual("\n" + word + "\n" + word, wordexp*2)
		self.assertToksEqual(word + "\n\n" + word, wordexp*2)

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


