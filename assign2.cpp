/*  Author: Harlan Chang & Cindy Quach
*   Created: April 18, 2020
*   
*   This program is a syntax analyzer that will prompt the user for
*   a file name input. It will output a 2 column list of Lexemes
*   and Tokens and analyze the syntax under each row.
*
*/

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <cctype>
#include <vector>

using namespace std;

// transition states in the fsm
enum FSM_Transitions
{
    RESET = 0,
    //we have no way to check if it's an identifier or keyword while we're going through the characters, so we will just use string as temp
    STRING,
    SEPARATOR,
    OPERATOR,
    INTEGER,
    REAL,
    //we only use these two after we get string and check what it is 
    KEYWORD,
    IDENTIFIER
};

// struct to hold token information
struct token_Info
{
    string token;
    int lexeme;
    string lexemeName;
};

// FSM Table
int FSM_Table[][7] = {{RESET, STRING, SEPARATOR, OPERATOR, INTEGER, REAL}, //RESET STATE
/* STATE 1 */   {STRING, STRING, RESET, RESET, STRING, RESET}, //PREV WAS STRING
/* STATE 2 */   {SEPARATOR, RESET, RESET, RESET, RESET, RESET}, //PREV WAS SEPARATOR
/* STATE 3 */   {OPERATOR, RESET, RESET, RESET, RESET, RESET}, //PREV WAS OPERATOR
/* STATE 4 */   {INTEGER, RESET, RESET, RESET, INTEGER, REAL}, //PREV WAS INT
/* STATE 5 */   {REAL, RESET, RESET, RESET, RESET, REAL}}; //PREV WAS REAL


bool isKeyword(string myLexeme)
{
    //compare the lexeme with our keywords
    if((myLexeme.compare("int") == 0)||(myLexeme.compare("float") == 0)||(myLexeme.compare("bool") == 0)||(myLexeme.compare("true") == 0)
    ||(myLexeme.compare("false") == 0)||(myLexeme.compare("if") == 0)||(myLexeme.compare("else") == 0)||(myLexeme.compare("then") == 0)
    ||(myLexeme.compare("endif") == 0)||(myLexeme.compare("while") == 0)||(myLexeme.compare("whileend") == 0)||(myLexeme.compare("do") == 0)
    ||(myLexeme.compare("doend") == 0)||(myLexeme.compare("for") == 0)||(myLexeme.compare("forend") == 0)||(myLexeme.compare("input") == 0)
    ||(myLexeme.compare("output") == 0)||(myLexeme.compare("and") == 0)||(myLexeme.compare("or") == 0)||(myLexeme.compare("not") == 0))
    {
        return true;
    }
    return false;
}

bool isIdentifier(string myLexeme)
{
    //the first value MUST be an alphabetical character
    if(!isalpha(myLexeme[0]))
    {
        return false;
    }
    //go through each character in the string to check if it's valid
    for(int i = 1; i < myLexeme.length(); i++)
    {
        //checks if it has one of the invalid characters for indentifiers
        if(!isalpha(myLexeme[i])&&!isdigit(myLexeme[i])&&myLexeme[i] != '$')
        {
            return false;
        }
    }
    return true;
}

bool isSeparator(char myLexeme)
{
    //return true if the lexeme is one of the separators, otherwise return false
    if((myLexeme == '\'')||(myLexeme == '(')||(myLexeme == ')')||(myLexeme == '{')||(myLexeme == '}')||(myLexeme == '[')||
    (myLexeme == ']')||(myLexeme == ',')||(myLexeme == ',')||(myLexeme == '.')||(myLexeme == ':')||(myLexeme == ';'))
    {
        return true;
    }
    return false;
}

bool isOperator(char myLexeme)
{
    //return true if the lexeme is one of the operators, otherwise return false
    if((myLexeme == '*')||(myLexeme == '+')||(myLexeme == '-')||(myLexeme == '=')||(myLexeme == '/')||(myLexeme == '>')||
    (myLexeme == '<')||(myLexeme == '%'))
    {
        return true;
    }
    return false;
}

int checkChar(int prevState, char currChar)
{

    if(prevState == STRING)
    {
        //if the previous state is a string, these are the valid next states
        if(isalpha(currChar) || currChar == '$' || isdigit(currChar))
        {
            return STRING;
        }
        
    }
    else if(prevState == INTEGER)
    {
        //to differentiate an integer and a real
        if(isdigit(currChar))
        {
            return INTEGER;
        }
        else if(currChar == '.')
        {
            return REAL;
        }
        
    }
    else if(prevState == REAL)
    {
        //continue as a real instead of integer
        if(isdigit(currChar))
        {
            return REAL;
        }
        //we don't need to check for . here because there can only be one in the number
        
    }
    
    //if the previous isnt String, Integer, or Real, we can just check regularly
    if(isalpha(currChar))
    {
        return STRING;
    }
    else if(isSeparator(currChar))
    {
        return SEPARATOR;
    }
    else if(isOperator(currChar)) 
    {
       return OPERATOR;
    }
    else if(isdigit(currChar))
    {
        return INTEGER;
    }

    return RESET;
}

string returnLexName(int lexeme)
{
    //get the name of the lexeme based off the number
    switch(lexeme)
    {
        case KEYWORD:
           return "KEYWORD";
           break;
        case IDENTIFIER:
           return "IDENTIFIER  ";
           break;
        case SEPARATOR:
           return "SEPARATOR";
           break;
        case OPERATOR:
           return "OPERATOR";
           break;
        case INTEGER:
            return "INTEGER";
            break;
        case REAL:
           return "REAL";
           break;
        case STRING:
            return "STRING";
            break;
        case RESET:
            return "RESET";
            break;
        default:
           return "ERROR";
           break;
    }
}

vector<token_Info> lexer(string myString)
{
    token_Info myToken; //this is going to be used to hold every token
    vector<token_Info> tokens; //this is going to be used to return all the tokens back
    int column = RESET; //this is the column that we're going to be in
    int prevState = RESET; //this holds the previous state
    int currentState = RESET; //this holds the current state
    char currentChar = ' '; //this holds the current character that we're trying to analyze
    string currentStr = ""; //this holds the current string that we're trying to analyze 
    bool ignoreBlock = false; //this is used to check the comment block and ignore the characters if its still valid

    //we're going to loop through the entire string and check every character
    for(int i = 0; i < myString.length();i++)
    {
        //get the current character
        currentChar = myString[i];
        
        //checks if there is a comment block 
        if(currentChar == '!')
        {
            if(ignoreBlock)
            {
                //so we don't go out of bound
                if(i != myString.length() - 1)
                {
                    ignoreBlock = false;
                }
            }
            else
            {
                ignoreBlock = true;
            }
            
        }

        //if the comment block isn't active
        if(!ignoreBlock)
        {
            //get the column for the FSM state
            column = checkChar(currentState, currentChar);
            //get the new current state based off the current state and the column
            currentState = FSM_Table[currentState][column];


            //if we reached a reset
            if(currentState == RESET)
            {
                //check if it's a string so that we can differentiate which string 
                if(prevState == STRING)
                {
                    if(isKeyword(currentStr))
                    {
                        prevState = KEYWORD;
                    }
                    else if(isIdentifier(currentStr))
                    {
                        prevState = IDENTIFIER;
                    }
                }
                //only set it as a token if the current is a RESET and the previous wasn't
                if(prevState != RESET)
                {
                    myToken.token = currentStr;
                    myToken.lexeme = prevState;
                    myToken.lexemeName = returnLexName(myToken.lexeme);
                    tokens.push_back(myToken);
                    currentStr = "";
                }

                //decrement if the RESET isn't a space so that we can analyze it
                if(!isspace(currentChar))
                {
                    i--;
                }
            }
            else
            {
                currentStr += currentChar;
                //add the value into the current string to be set as a token
            }

            prevState = currentState;
        }
    }

    //make sure the current string isn't empty for the last token
    if(currentStr != "")
    {
        //same stuff as the above section where we created the tokens
        if(prevState == STRING)
        {
            if(isKeyword(currentStr))
            {
                currentState = KEYWORD;
            }
            else if(isIdentifier(currentStr))
            {
                currentState = IDENTIFIER;
            }
        }
        if(currentState != RESET)
        {
            myToken.token = currentStr;
            myToken.lexeme = currentState;
            myToken.lexemeName = returnLexName(myToken.lexeme);
            tokens.push_back(myToken);
        }
    }

    return tokens;
}

void terms(token_Info token)
{
    if(isIdentifier(token.token))
    {
        cout << "<Term> -> <Factor> <Term Prime>\n<Factor> -> <Identifier>" << endl;
        return;
    }
    else
    {
        cout << "Syntax Invalid!!";
        exit(0);
    }
    
}

void termPrime(vector<token_Info> tokens)
{
    if(tokens[0].token == "*" || tokens[0].token == "/")
    {   
        cout << "<Empty> -> Epsilon\n<TermPrime> -> * <Factor> <TermPrime> | / <Factor> <TermPrime> | <Empty>" << endl;
        tokens.erase(tokens.begin());
        terms(tokens[0]);
    }
    else
    {
        cout << "<Empty> -> Epsilon" << endl;
        return;
    }
    
}

void expressionsPrime(vector<token_Info> tokens)
{
    if(tokens[0].token == "*" || tokens[0].token == "/" || tokens[0].token == ";")
    {
        cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
        termPrime(tokens);
    }

    if(tokens[0].token == "+" || tokens[0].token == "-")
    {
        cout << "<ExpressionPrime> -> + <Term> <ExpressionPrime> | - <Term> <ExpressionPrime> | <Empty> " << endl;
        
        tokens.erase(tokens.begin());
        cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
        terms(tokens[0]);

        tokens.erase(tokens.begin());
        cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
        expressionsPrime(tokens);
    }
    else
    {
        cout << "<Empty> -> Epsilon" << endl;
        return;
    }
    
}

void expressions(vector<token_Info> tokens)
{
    cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
    cout << "<Expression> -> <Term> <Expression Prime>" << endl;
    terms(tokens[0]);

    tokens.erase(tokens.begin());
    cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
    expressionsPrime(tokens);

    cout << "<Empty> -> Epsilon" << endl;
    return;
}

void checkSyntax(vector<token_Info> tokens)
{
    //need to get the first identifier otherwise we can ignore like %% in sample
    while(!isIdentifier(tokens[0].token))
    {
        //print out the first token and lexeme
        cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
        //get rid of the character in the vector
        tokens.erase(tokens.begin());
    }

    //we know this is an identifier
    cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;
    cout << "<Statement> -> <Assign>\n<Assign> -> <Identifier> = <Expression> ;" << endl;
    tokens.erase(tokens.begin());

    //we know that the next one should be assignment operator
    if(tokens[0].token == "=")
    {
        cout << "Token: " << tokens[0].lexemeName << "\t\t" << "Lexeme: " << tokens[0].token << endl;   
        tokens.erase(tokens.begin());
    }
    else
    {
        cout << "Syntax invalid!!";
        exit(0);
    }
    
    expressions(tokens);
}

int main () {

    ifstream infile;
    ofstream outfile;
    string fileName;
    string singleLine;
    vector<token_Info> tokens;

    // get input file from user
    cout<<"Please enter the name of the file: ";
    getline(cin, fileName);
    infile.open(fileName.c_str());
    outfile.open("outputfile.txt");

    if(infile.fail())
    {
        cout<< fileName << "not found!";
        exit(1);
    }
    
    outfile << "LEXEMES \t\t" << "TOKENS" << endl << endl;

    //goes through every line of the input file
    while(getline(infile, singleLine))
    {
        //gets the tokens from lexer
        tokens = lexer(singleLine);
        //pass just the tokens because they're already split up
        checkSyntax(tokens);

    }

    infile.close();
    outfile.close();

    return 0;
}