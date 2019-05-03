#include "ui.h"
  

UI::UI(QWidget *parent)
	: QWidget(parent)
{
	this->setWindowTitle("JYH's plot function");
	ui.setupUi(this);
	qwtPlot = nullptr;
	connect(ui.btn_cancel, SIGNAL(clicked()), this, SLOT(quit()));
	connect(ui.btn_plot, SIGNAL(clicked()), this, SLOT(procedure()));
	connect(ui.btn_clr, SIGNAL(clicked()), this, SLOT(clear()));
	clear();
}

void UI::quit() {
	close();
}

void UI::procedure() {
	if (!qwtPlot) {
		qwtPlot = new QwtPlot(this);
		qwtPlot->setObjectName(QStringLiteral("qwtPlot"));
		qwtPlot->setGeometry(QRect(240, 80, 400, 200));
	}
	QString cmd = ui.cli->toPlainText();
	QRegularExpression pattern ("(//|--).*\n");
	commands.clear();
	//Regular expressions are actually cheating
	cmd = cmd.replace(pattern, " ");//remove quotations using RegularExpression
	cmd = cmd.replace("\n", " ");
	if (cmd.contains("//")) 
		cmd = cmd.split("//")[0];//remove trailing //
	if (cmd.contains("--"))
		cmd = cmd.split("--")[0];//remove trailing --  

	commands = cmd.toStdString();
	transform(commands.begin(), commands.end(), commands.begin(), toupper);
	cur = 0;
	if (FetchToken()) {//��ȡһ���Ǻ�
		return;
	}
	Program();
}

void UI::ErrMsg(char* descrip, char* string) {
	char msg[256] {0};
	QMessageBox::information(this, descrip, string);
}

void UI::clear() {
	cur = 0;
	commands.clear();
	ui.cli->clear();
	scaleX = 1;
	scaleY = 1;
	originx = 0;
	originy = 0;
	rot_angle = 0;
	memset(&token, 0, sizeof(token));
	for(auto p:curves) {
		p->detach();
	}
	for(auto & p:curves) {
		delete p;
	}
	curves.clear();
	if (!qwtPlot) {
		delete qwtPlot;
		qwtPlot = nullptr;
	}
}

int UI::SyntaxError(int case_of) {
	switch (case_of){
	case 1: ErrMsg("Error Token ", token.lexeme);
		break;
	case 2: ErrMsg("Unexpected token", token.lexeme);
		break;
	default: break;
	}
	return 0;
}

char UI::getc() {
	char x = commands[cur];
	cur++;
	return x;
}

int UI::ungetc() {
	if (cur > 0)
		cur--;
	return 0;
} 

Token UI::getTokenType(char* buf) {
	int loop;
	for (loop = 0; loop < sizeof(TokenTab) / sizeof(TokenTab[0]); loop++)
	{	//����TokenTab��,�ҵ��˾ͷ�����ȷ�ļǺ�
		if (!strcmp(TokenTab[loop].lexeme, buf))
			return TokenTab[loop];
	}
	Token errortoken;
	memset(&errortoken, 0, sizeof(Token));
	errortoken.type = ERRTOKEN;
	return errortoken;//���س���Ǻ�
}

int  UI::Program() {
	int ret = 0;
	while(token.type!= NONTOKEN) { 
		if (Statement())
			return 1;
	}
	return 0;
}

int UI::Statement() {
	int ret;
	switch (token.type)
	{
	case ORIGIN: ret = OriginStatement(); break;
	case SCALE: ret = ScaleStatement(); break;
	case ROT:  ret = RotStatement(); break;
	case FOR: ret = ForStatement(); break;
	default: SyntaxError(2); return 1; //���򱨴�
	}
	return ret;
}

double UI::getExprValue(struct ExprNode *root)//�����Ǳ��ʽ�ĸ�
{//���������﷨��  ���ݲ�ͬ�Ľڵ����ͼ��㵱ǰ���ڵ��ֵ
	if (root == NULL) return 0.0;
	switch (root->Opcode)
	{
		//��Ԫ�����
	case PLUS:
		return getExprValue(root->Content.CaseOperator.Left) +
			getExprValue(root->Content.CaseOperator.Right);
	case MINUS:
		return getExprValue(root->Content.CaseOperator.Left) -
			getExprValue(root->Content.CaseOperator.Right);
	case MUL:
		return getExprValue(root->Content.CaseOperator.Left) *
			getExprValue(root->Content.CaseOperator.Right);
	case DIV:
		return getExprValue(root->Content.CaseOperator.Left) /
			getExprValue(root->Content.CaseOperator.Right);
	case POWER:
		return pow(getExprValue(root->Content.CaseOperator.Left),
			getExprValue(root->Content.CaseOperator.Right));
	case FUNC:
		return(*root->Content.CaseFunc.funcptr)
			(getExprValue(root->Content.CaseFunc.Child));
	case CONST_ID:
		return root->Content.CaseConst;
	case T:
		return *(root->Content.CaseParmPtr);
	default:
		return 0.0;
	}//����ֵ�Ǳ��ʽ��ֵ
}

Token_t UI::GetToken() {
	Token token;
	char ch;
	char tokenbuffer[100] {0};
	memset(&token, 0, sizeof(token));
	while (true) {
		ch = getc();
		if (cur == commands.length())//������
		{
			token.type = NONTOKEN;
			return token;
		}
		if (!isspace(ch)) {
			AddChar(tokenbuffer, ch);
			break;
		}
	}
	if (isalpha(ch)) {//�������ĸ��������Ǳ����֡�������PI��E��T
		while (true) {
			ch = getc();
			if (isalnum(ch)) {
				AddChar(tokenbuffer, ch);
			}
			else {
				ungetc();
				break;
			}
		}
		token = getTokenType(tokenbuffer);
		strcpy(token.lexeme, tokenbuffer);
		return token;
	}
	else if(isdigit(ch)){//��������֣���ֻ���������֡�С��
		while (true) {
			ch = getc();
			if (isdigit(ch)) {
				AddChar(tokenbuffer, ch);
			}
			else
				break;
		}
		if (ch == '.') {
			AddChar(tokenbuffer, ch);
			while (true) {
				ch = getc();
				if (isdigit(ch)) {
					AddChar(tokenbuffer, ch);
				}
				else break;
			}
		}
		else
			ungetc();
		token.type = CONST_ID;//һ���ǳ���
		strcpy(token.lexeme, tokenbuffer);
		token.value = atof(tokenbuffer);
		return token;
	}
	else {
		switch(ch) {
		case ';':token.type = SEMICO; AddChar(tokenbuffer, ch); break;
		case '(':token.type = L_BRACKET; AddChar(tokenbuffer, ch); break;
		case ')':token.type = R_BRACKET; AddChar(tokenbuffer, ch); break;
		case '+':token.type = PLUS; AddChar(tokenbuffer, ch); break;
		case '/':token.type = DIV; AddChar(tokenbuffer, ch); break;
		case ',': token.type = COMMA; AddChar(tokenbuffer, ch); break;
		case '-':token.type = MINUS; AddChar(tokenbuffer, ch); break;
		case '*':
			AddChar(tokenbuffer, ch);
			ch = getc();
			if(ch == '*') {
				token.type = POWER;
				AddChar(tokenbuffer, ch);
				break;
			}
			token.type = MUL;
			ungetc();
			break;
		default:token.type = NONTOKEN; break;
		}
		strcpy(token.lexeme, tokenbuffer);
		return token;
	}
}

int UI::FetchToken() {
	token = GetToken();
	if (token.type == NONTOKEN && cur < commands.length()) {
		SyntaxError(2);
		return 1;
	}
	return 0;
}

int UI::MatchToken(Token_Type The_Token) {
	if (cur == commands.length())
		return 0;
	if(token.type != The_Token) {
		SyntaxError(2);
		return 1;
	}
	if(cur < commands.length())
		return FetchToken();//�õ���һ��token
	return 0;
}

void UI::AddChar(char *buf, char ch) {//�ѷ���ch���뵽������buf��
	int i = 0;
	for (i = 0; buf[i] != 0; i++) ;
	buf[i] = ch;
}

int UI::DelExprTree(ExprNode* root) {
	if (root == nullptr) return 0;
	switch (root->Opcode)
	{
	case PLUS://��Ԫ�����������ӵ��ڲ��ڵ�
	case MINUS:
	case MUL:
	case DIV:
	case POWER:
		DelExprTree(root->Content.CaseOperator.Left);
		DelExprTree(root->Content.CaseOperator.Right);
		break;
	case FUNC:
		DelExprTree(root->Content.CaseFunc.Child);//һԪ����һ�����ӵ��ڲ��ڵ�
		break;
	default:
		break;
	}
	delete root;
	return 0;
}

struct ExprNode* UI::Expression() {
	struct ExprNode *left, *right;//���������ڵ�ָ��
	Token_Type token_tmp;//��ǰ�Ǻ�

	left = Term();//������������õ����﷨�������������һ���˳�������ʽ
	while (token.type == PLUS || token.type == MINUS){
		token_tmp = token.type;//��ǰ�Ǻ��Ǽ�/��
		MatchToken(token_tmp);//ƥ��Ǻ�
		right = Term();//�����Ҳ������õ����﷨�����Ҳ�������һ���˳�������ʽ
		left = MakeExprNode(token_tmp, left, right);//����������﷨�������Ϊ������
	}
	return left;
}

struct ExprNode *UI::Term(){
	struct ExprNode *left, *right;
	Token_Type token_tmp;
	left = Factor();
	while (token.type == MUL || token.type == DIV)
	{
		token_tmp = token.type;
		MatchToken(token_tmp);
		right = Factor();
		left = MakeExprNode(token_tmp, left, right);
	}
	return left;
}

struct ExprNode* UI::MakeExprNode(enum Token_Type opcode, ...) {
	struct ExprNode *ExprPtr = new(struct ExprNode);//Ϊ�½ڵ㿪�ٿռ�
	ExprPtr->Opcode = opcode;//��ڵ�д��Ǻ����
	va_list ArgPtr;//ָ��ɱ亯���Ĳ�����ָ��
	va_start(ArgPtr, opcode);//��ʼ��va_list��������һ������Ҳ���ǹ̶�����Ϊopcode
	switch (opcode)//���ݼǺŵ�����첻ͬ�Ľڵ�
	{
	case CONST_ID://�����ڵ�
		ExprPtr->Content.CaseConst = (double)va_arg(ArgPtr, double);//���ؿɱ�������ɱ���������ǳ���
		break;
	case T://����T�ڵ�
		ExprPtr->Content.CaseParmPtr = &Parameter;//���ؿɱ�������ɱ���������ǲ���T
		break;
	case FUNC://�������ýڵ�
		ExprPtr->Content.CaseFunc.funcptr = (FuncPtr)va_arg(ArgPtr, FuncPtr);//�ɱ���������Ƕ�Ӧ������ָ��
		ExprPtr->Content.CaseFunc.Child = (struct ExprNode *)va_arg(ArgPtr, struct ExprNode *);//�ɱ���������ǽڵ�
		break;
	default://��Ԫ����ڵ�
		ExprPtr->Content.CaseOperator.Left = (struct ExprNode *)va_arg(ArgPtr, struct ExprNode *);//�ɱ���������ǽڵ�
		ExprPtr->Content.CaseOperator.Right = (struct ExprNode *)va_arg(ArgPtr, struct ExprNode *);//ͬ��
		break;
	}
	va_end(ArgPtr);//�����ɱ�����Ļ�ȡ

	return ExprPtr;
}

int UI::RotStatement() {
	struct ExprNode *tmp;
	if (MatchToken(ROT))
		return 1;
	if (MatchToken(IS)) 
		return 1;

	tmp = Expression();  //Tree_trace(tmp);
	if (!tmp)
		return 1; 
	rot_angle = getExprValue(tmp);//�����ת�Ƕ�
	DelExprTree(tmp);
	if (MatchToken(SEMICO))
		return 1;
	return 0;
}

int UI::ScaleStatement(void)
{
	struct ExprNode *tmp;
	if (MatchToken(SCALE))
		return 1;
	if (MatchToken(IS))
		return 1;
	if (MatchToken(L_BRACKET))
		return 1;

	tmp = Expression();  //Tree_trace(tmp);
	if (!tmp)
		return 1;
	scaleX = getExprValue(tmp);//��ȡ������ı�������
	DelExprTree(tmp);

	if (MatchToken(COMMA))
		return 1;

	tmp = Expression();    //Tree_trace(tmp);
	if (!tmp)
		return 1;
	scaleY = getExprValue(tmp);//��ȡ������ı�������
	DelExprTree(tmp);

	if (MatchToken(R_BRACKET))
		return 1;

	if (MatchToken(SEMICO))
		return 1;
	return 0;
}

int UI::ForStatement(void){//for T from Expression to Expression draw (Expression, Expression)
	double Start, End, Step;//��ͼ��㡢�յ㡢����
	struct ExprNode *start_ptr, *end_ptr, *step_ptr, *x_ptr, *y_ptr;
	if (MatchToken(FOR))
		return 1;
	if (MatchToken(T))
		return 1;
	if (MatchToken(FROM))
		return 1;
	start_ptr = Expression(); //Tree_trace(start_ptr);//��ò��������ʽ���﷨��
	if (!start_ptr)
		return 1;
	Start = getExprValue(start_ptr);//������������ʽ��ֵ
	DelExprTree(start_ptr);//�ͷŲ�������﷨����ռ�ռ�
	if (MatchToken(TO))
		return 1;
	end_ptr = Expression(); //Tree_trace(end_ptr);//��������յ���ʽ�﷨��
	if (!end_ptr)
		return 1;
	End = getExprValue(end_ptr);//��������յ���ʽ��ֵ
	DelExprTree(end_ptr);//�ͷŲ����յ��﷨����ռ�ռ�
	if (MatchToken(STEP))
		return 1;

	step_ptr = Expression(); //Tree_trace(step_ptr);//��������������ʽ�﷨��
	if (!step_ptr)
		return 1;
	Step = getExprValue(step_ptr);//��������������ʽ��ֵ
	DelExprTree(step_ptr);//�ͷŲ��������﷨����ռ�ռ�

	if (MatchToken(DRAW))
		return 1;
	if (MatchToken(L_BRACKET))
		return 1;

	x_ptr = Expression(); //Tree_trace(x_ptr);
	if (!x_ptr)
		return 1;

	if (MatchToken(COMMA))
		return 1;

	y_ptr = Expression();//Tree_trace(y_ptr);
	if (!y_ptr)
		return 1;

	if (MatchToken(R_BRACKET))
		return 1;

	if (MatchToken(SEMICO))
		return 1;
	DrawLoop(Start, End, Step, x_ptr, y_ptr); //����ͼ��
	DelExprTree(x_ptr);//�ͷź������﷨����ռ�ռ�
	DelExprTree(y_ptr);//�ͷ��������﷨����ռ�ռ�

	return 0;
}

int UI::OriginStatement() {
	struct ExprNode *tmp;//�﷨���ڵ������
	MatchToken(ORIGIN);
	if (MatchToken(IS)) {
		return 1;
	}
	if (MatchToken(L_BRACKET)) {
		return 1;
	}

	tmp = Expression();  //Tree_trace(tmp);
	if (!tmp)
		return 1;
	originx = getExprValue(tmp);//��ȡ�������ƽ�ƾ���
	DelExprTree(tmp);//ɾ��һ����

	if (MatchToken(COMMA))
		return 1;

	tmp = Expression();   //Tree_trace(tmp);
	if (!tmp)
		return 1;
	originy = getExprValue(tmp);//��ȡ�������ƽ�ƾ���
	DelExprTree(tmp);//ɾ��һ����

	if (MatchToken(R_BRACKET))
		return 1;

	if (MatchToken(SEMICO))
		return 1;
	return 0;
}

struct ExprNode *UI::Component()//�ҽ��
{
	struct ExprNode *left, *right;
	left = Atom();
	if (!left)
		return nullptr;
	if (token.type == POWER)
	{
		MatchToken(POWER);
		right = Component();//�ݹ����Component��ʵ��POWER���ҽ��
		left = MakeExprNode(POWER, left, right);
		if (!left)
			return nullptr;
	}
	return left;
}

struct ExprNode *UI::Atom(){
	struct Token t = token;
	struct ExprNode *address, *tmp;
	switch (token.type)
	{
	case CONST_ID:
		MatchToken(CONST_ID);
		address = MakeExprNode(CONST_ID, t.value);
		break;
	case T:
		MatchToken(T);
		address = MakeExprNode(T);
		break;
	case FUNC:
		MatchToken(FUNC);
		MatchToken(L_BRACKET);
		tmp = Expression();  //Tree_trace(tmp);
		address = MakeExprNode(FUNC, t.funcptr, tmp);
		MatchToken(R_BRACKET);
		break;
	case L_BRACKET:
		MatchToken(L_BRACKET);
		address = Expression(); //Tree_trace(address);
		MatchToken(R_BRACKET);
		break;
	default:
		SyntaxError(2);
		return nullptr;
	}
	return address;
}

ExprNode* UI::Factor() {
	struct ExprNode *left, *right;
	if (token.type == PLUS) {//ƥ��һԪ������
		MatchToken(PLUS);
		right = Factor();
	}
	else if (token.type == MINUS)//���ʽ�˻�Ϊ�����Ҳ������ı��ʽ
	{
		MatchToken(MINUS);
		right = Factor();
		left = new ExprNode;
		left->Opcode = CONST_ID;
		left->Content.CaseConst = 0.0;
		right = MakeExprNode(MINUS, left, right);
	}
	else right = Component();//ƥ����ս��Component
	return right;
}

void UI::DrawLoop(double Start,double End,double Step,struct ExprNode *HorPtr,struct ExprNode *VerPtr){
	double x, y;
	QVector<QPointF> pts;
	double tmp;
	for (Parameter = Start; Parameter <= End; Parameter += Step){
		QPointF point;
		x = y = 0;
		x = getExprValue(HorPtr);
		y = getExprValue(VerPtr);
		x *= scaleX;
		y *= scaleY;
		tmp = x * cos(rot_angle) + y * sin(rot_angle);
		y = y * cos(rot_angle) - x * sin(rot_angle);
		x = tmp;
		x += originx;
		y += originy;
		point.setX(x);
		point.setY(y);
		pts.append(point);
	}
	QwtPointSeriesData * data = new QwtPointSeriesData(pts);
	QwtPlotCurve *curve = new QwtPlotCurve("Curve Plot");
	curves.append(curve);
	curve->setData(data);
	curve->attach(qwtPlot);
	qwtPlot->replot();
	qwtPlot->show();
}