#include "ss_code_ide.hpp"
#include <cctype>

static std::vector<Token>	tokenizeCode(const std::string& code, Color keywordColour, Color typeColour, Color stringColour, Color textColour)
{
	std::vector<Token>	tokens;
	size_t				i(0);
	size_t				len(code.length());
	bool				isInclude(code.find("#include") != std::string::npos);
	t_text				space("");
	t_text				str("");
	size_t				closePos(0);
	t_text				headerContent("");
	t_text				op2("");
	char				c(0);
	t_text				word("");
	Color				cColor(textColour);
	bool				isBold(false);

	while (i < len)
	{
		if (std::isspace(code[i]))
		{
			space = "";
			while (i < len && std::isspace(code[i]))
			{
				space += code[i];
				i++;
			}
			tokens.push_back(Token{space, textColour, false});
			continue;
		}

		if (code[i] == '"')
		{
			str = "";
			str += code[i++];
			while (i < len && code[i] != '"')
			{
				if (code[i] == '\\' && i + 1 < len)
					str += code[i++];
				str += code[i++];
			}
			if (i < len && code[i] == '"')
				str += code[i++];
			tokens.push_back(Token{str, stringColour, false});
			continue;
		}

		if (isInclude && code[i] == '<')
		{
			closePos = code.find('>', i);
			if (closePos != std::string::npos)
			{
				headerContent = code.substr(i + 1, closePos - i - 1);
				tokens.push_back(Token{"<", keywordColour, true});
				tokens.push_back(Token{headerContent, stringColour, false});
				tokens.push_back(Token{">", keywordColour, true});
				i = closePos + 1;
				continue;
			}
		}

		op2 = (i + 1 < len) ? code.substr(i, 2) : "";
		if (op2 == "::" || op2 == "<<" || op2 == ">>" || op2 == "&&" || op2 == "||" || op2 == "==" || op2 == "!=" || op2 == "<=" || op2 == ">=")
		{
			tokens.push_back(Token{op2, keywordColour, true});
			i += 2;
			continue;
		}

		c = code[i];
		if (c == '<' || c == '>' || c == '{' || c == '}' || c == '(' || c == ')' || c == ',' || c == ';' || c == ':' || c == '!' || c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '&' || c == '.' || c == '~' || c == '[' || c == ']')
		{
			tokens.push_back(Token{std::string(1, c), keywordColour, false});
			i++;
			continue;
		}

		if (std::isalpha(c) || c == '_' || c == '#')
		{
			word = "";
			while (i < len && (std::isalnum(code[i]) || code[i] == '_' || code[i] == '#'))
			{
				word += code[i];
				i++;
			}

			cColor = textColour;
			isBold = false;
			if (word == "#include" || word == "class" || word == "public" || word == "private" || word == "const" || word == "if" || word == "throw" || word == "return")
			{
				cColor = keywordColour;
				isBold = true;
			}
			else if (word == "FileHandler" || word == "std" || word == "string" || word == "fstream" || word == "ios_base" || word == "openmode" || word == "runtime_error" || word == "cout" || word == "endl" || word == "open" || word == "is_open" || word == "close" || word == "get_file_stream")
			{
				cColor = typeColour;
				isBold = false; // standard library names standard regular
			}
			else if (word == "void" || word == "int" || word == "double" || word == "float" || word == "char" || word == "bool" || word == "t_text")
			{
				cColor = typeColour;
				isBold = true;
			}

			tokens.push_back(Token{word, cColor, isBold});
			continue;
		}

		tokens.push_back(Token{std::string(1, c), textColour, false});
		i++;
	}
	return tokens;
}

std::vector<Token> tokenize(const std::string& line, Color keywordColour, Color typeColour, Color stringColour, Color commentColour, Color textColour)
{
	size_t				commentStart(line.find("//"));
	t_text				codePart("");
	t_text				commentPart("");
	std::vector<Token>	tokens;
	std::vector<Token>	codeTokens;

	if (line.empty())
	{
		tokens.push_back(Token{"", textColour, false});
		return (tokens);
	}

	if (commentStart != std::string::npos)
	{
		codePart = line.substr(0, commentStart);
		commentPart = line.substr(commentStart);
		
		if (!codePart.empty())
		{
			codeTokens = tokenizeCode(codePart, keywordColour, typeColour, stringColour, textColour);
			tokens.insert(tokens.end(), codeTokens.begin(), codeTokens.end());
		}
		tokens.push_back(Token{commentPart, commentColour, false});
		return tokens;
	}

	return tokenizeCode(line, keywordColour, typeColour, stringColour, textColour);
}
