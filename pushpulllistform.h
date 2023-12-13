#ifndef PUSHPULLLISTFORM_H
#define PUSHPULLLISTFORM_H

#include <QWidget>

class MainWindow;

namespace Ui {
class PushPullListForm;
}

class PushPullListForm : public QWidget
{
    Q_OBJECT

public:
    explicit PushPullListForm(QWidget *parent = nullptr);
    ~PushPullListForm();
    void setMainWindowInstance(MainWindow *mw_instance);

private:
    Ui::PushPullListForm *ui;
    MainWindow *mw;
    void addToPushPullList();
    void removeFromPushPullList();
};

#endif // PUSHPULLLISTFORM_H
