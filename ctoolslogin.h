#ifndef CTOOLSLOGIN_H
#define CTOOLSLOGIN_H

#include <QDialog>

namespace Ui {
class CtoolsLogin;
}

class CtoolsLogin : public QDialog
{
    Q_OBJECT

public:
    explicit CtoolsLogin(QWidget *parent = 0);
    ~CtoolsLogin();
    //bool eventFilter(QObject *target, QEvent *e);
private slots:
    void on_btnOk_clicked();

private:
    Ui::CtoolsLogin *ui;
};

#endif // CTOOLSLOGIN_H
