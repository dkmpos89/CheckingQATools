#include "ctoolslogin.h"
#include "ui_ctoolslogin.h"
#include <QMessageBox>
#include <QDebug>
#include <QKeyEvent>


CtoolsLogin::CtoolsLogin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CtoolsLogin)
{
    ui->setupUi(this);
    //installEventFilter();
}

CtoolsLogin::~CtoolsLogin()
{
    delete ui;
}

void CtoolsLogin::on_btnOk_clicked()
{
    QMessageBox::information(this, "TEST", "Enter!");
}

/*
bool CtoolsLogin::eventFilter( QObject* target, QEvent* e )
{
    if (e->type() == QEvent::KeyPress) {
             QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
             qDebug("Ate key press %d", keyEvent->key());
             return true;
         } else {
             // standard event processing
             return QWidget::eventFilter(target, e);
         }
    return QWidget::eventFilter(target, e);
}
*/
