#include "ctoolslogin.h"
#include "ui_ctoolslogin.h"

CtoolsLogin::CtoolsLogin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CtoolsLogin)
{
    ui->setupUi(this);
}

CtoolsLogin::~CtoolsLogin()
{
    delete ui;
}
