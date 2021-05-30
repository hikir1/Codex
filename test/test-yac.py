import unittest
from context import codex
import codex.parser
import codex.lexer

word = "word"
word_exp = ["w", "o", "r", "d"]

class Test(unittest.TestCase):
	def setUp(self):
		lexer = codex.lexer.new_lexer()
		self.parser = codex.parser.new_parser()
	def test_empty(self):
		self.assertEqual(self.parser.parse(""), [])
	def test_space(self):
		self.assertEqual(self.parser.parse(" "), [])
	def test_multi_space(self):
		self.assertEqual(self.parser.parse("  "), [])
	def test_word(self):
		self.assertEqual(self.parser.parse(word), [[word_exp]])
	def test_word_space_before(self):
		self.assertEqual(self.parser.parse(" " + word), [[word_exp]])
	def test_word_space_after(self):
		self.assertEqual(self.parser.parse(word + " "), [[word_exp]])
	def test_double_word(self):
		self.assertEqual(self.parser.parse(word + " " + word), [[word_exp, word_exp]])
	def test_newline(self):
		self.assertEqual(self.parser.parse("\n"), [])
	def test_newline_with_space(self):
		self.assertEqual(self.parser.parse(" \n"), [])
		self.assertEqual(self.parser.parse("\n "), [])
	def test_empty_paragraph(self):
		self.assertEqual(self.parser.parse("\n\n"), [])
		self.assertEqual(self.parser.parse("\n\n\n"), [])
		self.assertEqual(self.parser.parse(" \n\n"), [])
		self.assertEqual(self.parser.parse("\n\n "), [])
		self.assertEqual(self.parser.parse("\n\n \n"), [])
		self.assertEqual(self.parser.parse("\n \n\n"), [])
	def test_double_empty_paragraph(self):
		self.assertEqual(self.parser.parse("\n\n \n\n"), [])
	def test_paragraph_after_word(self):
		self.assertEqual(self.parser.parse(word + "\n\n"), [[word_exp]])
		self.assertEqual(self.parser.parse(word + " \n\n"), [[word_exp]])
		self.assertEqual(self.parser.parse(word + "\n\n "), [[word_exp]])
	def test_paragraph_before_word(self):
		self.assertEqual(self.parser.parse("\n\n" + word), [[word_exp]])
		self.assertEqual(self.parser.parse("\n\n " + word), [[word_exp]])
		self.assertEqual(self.parser.parse(" \n\n" + word), [[word_exp]])
	def test_double_paragraph(self):
		self.assertEqual(self.parser.parse(word + "\n\n" + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + "\n\n " + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + " \n\n" + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + "\n\n\n" + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + "\n\n \n" + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + "\n \n\n" + word), [[word_exp], [word_exp]])
		self.assertEqual(self.parser.parse(word + " \n\n \n " + word), [[word_exp], [word_exp]])
