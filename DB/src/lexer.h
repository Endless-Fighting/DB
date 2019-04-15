#pragma once
#ifndef _LEXER_H
#define _LEXER_H
#include <variant>
#include <utility>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include "dbexception.h"
#include "util.h"

#undef NULL

namespace std {
	template<> struct hash<const std::string> {
		std::size_t operator()(const std::string& key) const {
			static const std::hash<std::string> _hash_obj;
			return _hash_obj(key);
		}
	};
}

namespace DB::lexer
{
	using namespace util;

	enum class numeric_type { INT };
	using numeric_t = std::tuple<const int, const numeric_type>;

	using identifier = const std::string;

	using string_literal_t = std::tuple<const std::string>;

	/*
	 * enum class `type` represents: 
	 *1. all of keywords
	 *2. operators
	 *3. others, like `,.;()[]{}`
	 */
	enum class type
	{
		/* arithmetic operator */
		ADD, SUB, MUL, DIV, MOD, 

		/* logic operator */
		NOT, AND, OR, 

		/* compare operator */
		EQ, NEQ, 
		LESS, GREATER, LEQ, GEQ, 

		/* assign operator */
		ASSIGN,

		/* Parser type */
		NUMBER_CONSTANT, IDENTIFIER, STR_LITERAL,

		/* numeric type */
		INT,

		/* special keyword */
		CHAR, VARCHAR, WILDCARD,
		NULL, DISTINCT, VALUES,
		CREATE, DROP, INSERT, DELETE, UPDATE, SELECT, 
		TABLE, FROM, WHERE, JOIN,
		ORDERBY, ASC, DESC, SET,
		DEFAULT, PRIMARY_KEY, REFERENCES, 

		/* other operator */
		COMMA, PERIOD, SEMICOLON,                    // ",", ".", ";"
		QUESTION, COLON,                             // "?", ":"
		LEFT_PARENTHESIS, RIGHT_PARENTHESIS,         // parenthesis      : "(", ")"
		LEFT_SQUARE_BRACKETS, RIGHT_SQUARE_BRACKETS, // square brackets  : "[", "]"
		LEFT_CURLY_BRACKETS, RIGHT_CURLY_BRACKETS,   // curly brackets   : "{", "}"

		__EOF__,
	};

	type num_t2type(numeric_type num_t) noexcept;
	static const std::unordered_map<std::string, type> keywords =
	{
		/* arithmetic operator */
		{ "+", type::ADD }, { "-", type::SUB }, { "*", type::MUL }, { "/", type::DIV }, { "%", type::MOD },

		/* logic operator */
		{ "NOT", type::NOT }, { "AND", type::AND }, { "OR", type::OR },

		/* compare operator */
		{ "==", type::EQ }, { "!=", type::NEQ },
		{ "<", type::LESS }, { ">", type::GREATER }, { "<=", type::LEQ }, { ">=", type::GEQ },

		/* assign operator */
		{ "=", type::ASSIGN },

		/* numeric type */
		{ "INT", type::INT }, 

		/* special keyword */
		{ "CHAR", type::CHAR }, { "VARCHAR", type::VARCHAR }, { "$", type::WILDCARD },
		{ "NULL", type::NULL }, { "DISTINCT", type::DISTINCT }, { "VALUES", type::VALUES },
		{ "CREATE", type::CREATE },{ "DROP", type::DROP },
		{ "INSERT", type::INSERT },{ "DELETE", type::DELETE },{ "UPDATE", type::UPDATE },{ "SELECT", type::SELECT },
		{ "TABLE", type::TABLE },{ "FROM", type::FROM },{ "WHERE", type::WHERE },{ "JOIN", type::JOIN },
		{ "ORDERBY", type::ORDERBY },{ "ASC", type::ASC },{ "DESC", type::DESC },{ "SET", type::SET },
		{ "DEFAULT", type::DEFAULT },{ "PK", type::PRIMARY_KEY },{ "REFERENCES", type::REFERENCES },

		/* other operator */
		{ ",", type::COMMA }, { ".", type::PERIOD }, { ";", type::SEMICOLON },
		{ "?", type::QUESTION }, { ":", type::COLON },
		{ "(", type::LEFT_PARENTHESIS },      { ")", type::RIGHT_PARENTHESIS },
		{ "[", type::LEFT_SQUARE_BRACKETS },  { "]", type::RIGHT_SQUARE_BRACKETS },
		{ "{", type::LEFT_CURLY_BRACKETS },   { "}", type::RIGHT_CURLY_BRACKETS },

		// { "$eof$", type::__EOF__ }, // no use
	};

	// for display
	std::string type2str(type _type) noexcept;
	static const std::unordered_map<type, std::string> keyword2str =
	{
		/* arithmetic operator */
		{ type::ADD, "+" }, { type::SUB, "-" }, { type::MUL, "*",  }, { type::DIV, "/",  }, { type::MOD, "%" },

		/* logic operator */
		{ type::NOT, "NOT" }, { type::AND, "AND" }, { type::OR, "OR" },

		/* compare operator */
		{ type::EQ , "==" }, { type::NEQ, "!=" },
		{ type::LESS , "<" }, { type::GREATER, ">" }, { type::LEQ , "<=" }, { type::GEQ, ">=" },

		/* assign operator */
		{ type::ASSIGN, "=" },

		/* numeric type */
		{ type::INT, "INT" }, 

		/* special keyword */
		{ type::CHAR, "CHAR" }, { type::VARCHAR, "VARCHAR" }, { type::WILDCARD, "WILDCARD" },
		{ type::NULL, "NULL" }, { type::DISTINCT, "DISTINCT" }, { type::VALUES, "VALUES" },
		{ type::CREATE, "CREATE" },{ type::DROP, "DROP" },
		{ type::INSERT, "INSERT" },{ type::DELETE, "DELETE" },{ type::UPDATE, "UPDATE" },{ type::SELECT, "SELECT" },
		{ type::TABLE, "TABLE" },{ type::FROM, "FROM" },{ type::WHERE, "WHERE" },{ type::JOIN, "JOIN" },
		{ type::ORDERBY, "ORDERBY" },{ type::ASC, "ASC" },{ type::DESC, "DESC" },{ type::SET, "SET" },
		{ type::DEFAULT, "DEFAULT" },{ type::PRIMARY_KEY, "PRIMARY_KEY" },{ type::REFERENCES, "REFERENCES" },

		/* other operator */
		{ type::COMMA, "," }, { type::PERIOD, "." }, {  type::SEMICOLON, ";" },
		{ type::QUESTION, "?" }, { type::COLON, ":" },
		{ type::LEFT_PARENTHESIS, "(" },      { type::RIGHT_PARENTHESIS, ")" },
		{ type::LEFT_SQUARE_BRACKETS, "[" },  { type::RIGHT_SQUARE_BRACKETS, "]" },
		{ type::LEFT_CURLY_BRACKETS, "{" },   { type::RIGHT_CURLY_BRACKETS, "}" },

		{ type::__EOF__, "$eof$" },
	};


	/*
	 * represent all types of token
	 */
	using token_t = const std::variant<type, identifier, numeric_t, string_literal_t>;


	/*
	 * Parameter:
	 *     s         :  char array to be processed.
	 *     size      :  actual amounts of char to be processed.
	 *
	 * Return value:
	 *     return a vector with tokens, otherwise the error message.
	 *
	 * Exception:
	 *     No exception thrown, all exception should be diagnoed innerly,
	 *     and return error message if possible.
	 *
	 */
	namespace analyzers
	{
		struct Token_Ex
		{
			std::string _msg;
			std::size_t _position;
			Token_Ex(const std::string& msg, std::size_t position)
				:_msg(msg), _position(position) {}
			Token_Ex(std::string&& msg, std::size_t position)
				:_msg(std::move(msg)), _position(position) {}
		};
	}
	using pos_t = const std::size_t;
	using token_info = std::tuple<token_t, pos_t>;
	std::variant<std::vector<token_info>, analyzers::Token_Ex> tokenize(const char* s, const std::size_t size) noexcept;


#include <cassert>
	struct Token
	{
		pos_t _pos;
		token_t _token;
		Token(token_info const& _token_info)
			:
			_pos(std::get<pos_t>(_token_info)),
			_token(std::get<token_t>(_token_info))
		{}
		Token(const Token&) = default;
		Token& operator=(const Token&) = default;
		Token(Token&&) = default;
		Token& operator=(Token&& other)
		{
			std::cerr << "WTF" << std::endl;
			assert(false);
			return *this;
		}
	};


	lexer::type getType(const Token& token);


	class Lexer
	{
	public:
		void tokenize(const std::string filename);
		std::size_t size() const;
		const Token& operator[](std::size_t pos) const { return _token_stream[pos]; }
		bool empty() const;
		Token getToken();       // only get, if no token, `throw DB_Universal_Exception`
		void popToken();        // pop front, if no token, `throw DB_Universal_Exception`
		Token consumeToken();   // consume, if no token, `throw DB_Universal_Exception`
		void print(std::ostream& out) const;     // for DEBUG
		Lexer() = default;
		Lexer(const Lexer&) = delete;
		Lexer& operator=(const Lexer&) = delete;
	private:
		std::deque<Token> _token_stream;
		std::size_t cur_pos = 0;
	};


}// end namespace DB::lexer

#endif // !_LEXER_H
