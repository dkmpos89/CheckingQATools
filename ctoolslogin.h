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

private:
    Ui::CtoolsLogin *ui;
};

#endif // CTOOLSLOGIN_H
