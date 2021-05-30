import unittest
import tempfile
from context import codex
import codex.html
import codex.parser
from codex.parser import Codex, Paragraph, Word

head = "<!DOCTYPE html><html><head></head><body>"
tail = "</body></html>"

def p(text):
	return f"<p>{text}</p>"

word = "word"
ast_word = Word(["w", "o", "r", "d"])

class Test(unittest.TestCase):
	def setUp(self):
		self.f = tempfile.TemporaryFile("w+")
		self.parser = codex.parser.new_parser()
	def tearDown(self):
		self.f.close()
	def assertHtmlEqual(self, ast, expected):
		codex.html.compile(ast, self.f)
		self.f.seek(0)
		self.assertEqual(self.f.read(), head + expected + tail)
	def test_empty(self):
		self.assertHtmlEqual(Codex(), "")
	def test_word(self):
		self.assertHtmlEqual(Codex([Paragraph([ast_word])]), p(word))
	def test_double_word_same_paragraph(self):
		self.assertHtmlEqual(Codex([Paragraph([ast_word, ast_word])]), p(word + " " + word))
	def test_double_paragraph(self):
		self.assertHtmlEqual(Codex([Paragraph([ast_word]), Paragraph([ast_word])]), p(word) + p(word))
	def test_escaped_chars(self):
		self.assertHtmlEqual(Codex([Paragraph([Word(["<", ">", "&"])])]), p("&lt;&gt;&amp;"))
		
