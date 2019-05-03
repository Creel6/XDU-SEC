#pragma once

#include <QtWidgets/QWidget>
#include <cmath>
#include <cstdlib>
#include <string>
#include <QMessageBox>
#include <QRegularExpression>
#include <algorithm>
#include <QWT/qwt_plot.h> 
#include <QWT/qwt_plot_curve.h>  
#include <QList>
#include <QtMath>
#include "ui_ui.h"
using namespace std;

typedef double(*FuncPtr)(double);
enum Token_Type {
	ORIGIN, SCALE, ROT, IS, TO,
	STEP, DRAW, FOR, FROM, T,
	SEMICO,L_BRACKET, R_BRACKET, COMMA, 
	PLUS, MINUS, MUL, DIV, POWER,
	FUNC, CONST_ID, NONTOKEN, ERRTOKEN 
};


typedef struct Token{
	Token_Type type;
	char lexeme[100];
	double value;
	double(*funcptr)(double);
}Token_t;


static Token TokenTab[] = //���ű�����
{
	{ CONST_ID,	"PI", 3.1415926,	nullptr },
	{ CONST_ID, "E",	2.71828,	nullptr },
	{ T,		"T",		0.0,	nullptr },
	{ FUNC,		"SIN",		0.0,	sin },
	{ FUNC,		"COS",		0.0,	cos },
	{ FUNC,		"TAN",		0.0,	tan },
	{ FUNC,		"LN",		0.0,	log },
	{ FUNC,		"EXP",		0.0,	exp },
	{ FUNC,		"SQRT",		0.0,	sqrt },
	{ ORIGIN,	"ORIGIN",	0.0,	nullptr },
	{ SCALE,	"SCALE",	0.0,	nullptr },
	{ ROT,		"ROT",		0.0,	nullptr },
	{ IS,		"IS",		0.0,	nullptr },
	{ FOR,		"FOR",		0.0,	nullptr },
	{ FROM,		"FROM",		0.0,	nullptr },
	{ TO,		"TO",		0.0,	nullptr },
	{ STEP,		"STEP",		0.0,	nullptr },
	{ DRAW,		"DRAW",		0.0,	nullptr },
};

struct ExprNode {
	enum Token_Type Opcode;
	union {
		struct {
			ExprNode *Left, *Right;
		}CaseOperator;
		struct {
			ExprNode * Child;
			double(*funcptr)(double);
		}CaseFunc;
		double CaseConst;
		double *CaseParmPtr;
	}Content;
};

class UI : public QWidget
{
	Q_OBJECT

public:
	UI(QWidget *parent = Q_NULLPTR);

public slots:
	void quit();
	void clear();
	void procedure();

private:
	Ui::UIClass ui;
	QwtPlot *qwtPlot;
	string commands;
	Token_t token;
	QList<QwtPlotCurve*> curves;
	int cur;
	double scaleX, scaleY;
	double originx, originy;
	double rot_angle;
	double Parameter;
	char getc();//�������ж�ȡһ���ַ���ͬʱ�����������һλ
	int ungetc();//����ǰ��������Ĺ��λ��ǰ��һλ
	int SyntaxError(int);//�﷨����
    static Token getTokenType(char*);//�ж�token����
	int Program();//����
	int Statement();//���
	int OriginStatement();//Origin���
	int RotStatement();//Rot���
	int ScaleStatement();//Scale���
	int ForStatement();//For���
	struct ExprNode *Expression();//���ʽ����Ԫ�Ӽ�������ʽ
	struct ExprNode *Term();//�˳�������ʽ
	struct ExprNode *Factor();//һԪ�Ӽ�������ʽ
	struct ExprNode *Component();//��������ʽ
	struct ExprNode *Atom();//ԭ�ӱ��ʽ

	int FetchToken();//���ôʷ���������GetToken���ѵõ��ĵ�ǰ��¼��������������õ��ļǺ��ǷǷ�����errtoken����ָ��һ���﷨����
	int MatchToken(enum Token_Type The_Token);//ƥ�䵱ǰ�Ǻ�
	void ErrMsg(char *descrip, char *string);//��ӡ������Ϣ

	double getExprValue(struct ExprNode*);//�����﷨���õ�ֵ
	static void AddChar(char*, char);//���ַ�����lexeme��
	int DelExprTree(struct ExprNode*);//ɾ��һ����
	struct ExprNode * MakeExprNode(enum Token_Type opcode, ...);//��������һ���ڵ�
	Token_t GetToken();//�õ���һ��token
	void DrawLoop(double Start,double End,	double Step, struct ExprNode *HorPtr,struct ExprNode *VerPtr);//��ͼ
};
