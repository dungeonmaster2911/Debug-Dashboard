#include "pushpulllistform.h"
#include "qmessagebox.h"
#include "ui_pushpulllistform.h"
#include "mainwindow.h"

PushPullListForm::PushPullListForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PushPullListForm)
{
    ui->setupUi(this);
    QObject::connect(ui->btn_pushpull_add_to_list,&QPushButton::clicked,this,&PushPullListForm::addToPushPullList);
    QObject::connect(ui->btn_pushpull_remove_from_list,&QPushButton::clicked,this,&PushPullListForm::removeFromPushPullList);
}

PushPullListForm::~PushPullListForm()
{
    delete ui;
}

void PushPullListForm::setMainWindowInstance(MainWindow *mw_instance){
    mw = mw_instance;
    QJsonArray arr = mw->config_json.value("push_list").toArray();
    foreach (auto item, arr) {
        ui->comboBox_pushpull_remove->addItem(item.toObject()["name"].toString());
    }
}

void PushPullListForm::addToPushPullList(){
    QString bin_name = ui->lineEdit_pushpull_add_bin_name->text();
    QString bin_repo_path = ui->lineEdit_pushpull_add_bin_repo_path->text();
    QString bin_target_path = ui->lineEdit_pushpull_add_bin_target_path->text();
    QMessageBox msgBox;

    if(bin_name.isEmpty() || bin_repo_path.isEmpty() || bin_target_path.isEmpty()) {
        msgBox.setText("Please enter all data correctly");
        msgBox.exec();
        return;
    }
    else{
        auto pushpull_list_data = QJsonObject({
                                            qMakePair("name",bin_name),
                                            qMakePair("repo_path",bin_repo_path),
                                            qMakePair("push_path",bin_target_path)
                                        });

        QJsonArray arr = mw->config_json.value("push_list").toArray();
        arr.append(pushpull_list_data);
        mw->config_json["push_list"] = arr;

        QJsonDocument doc(mw->config_json);

        if(mw->updateJson(doc)){
            mw->fillPushPullList(1);
            ui->lineEdit_pushpull_add_bin_name->clear();
            ui->lineEdit_pushpull_add_bin_repo_path->clear();
            ui->lineEdit_pushpull_add_bin_target_path->clear();
            msgBox.setText("List is updated");
            msgBox.exec();
            ui->comboBox_pushpull_remove->clear();
            foreach (auto btns, arr) {
                ui->comboBox_pushpull_remove->addItem(btns.toObject()["name"].toString());
            }
            this->hide();
        }
        else{
            msgBox.setText("Failed!! could not update the list");
            msgBox.exec();
        }
    }
}

void PushPullListForm::removeFromPushPullList(){
    QString item = ui->comboBox_pushpull_remove->currentText();
    QJsonArray arr = mw->config_json.value("push_list").toArray();
    int index = 0;
    bool item_found = false;

    QMessageBox msgBox;
    for(auto val: arr){
        if(val.toObject()["name"].toString() == item){
            item_found = true;
//            updateCommandOutput("btn found");
            arr.removeAt(index);
            mw->config_json["push_list"] = arr;
            QJsonDocument doc(mw->config_json);
            if(mw->updateJson(doc)){
                msgBox.setText("Item removed.\n UI will update after app restart");
                msgBox.exec();
                ui->comboBox_pushpull_remove->clear();
                foreach (auto btns, arr) {
                    ui->comboBox_pushpull_remove->addItem(btns.toObject()["name"].toString());
                }
            }
            else{
                msgBox.setText("Failed!! Could not remove item");
                msgBox.exec();
            }
            break;
        }
        index++;
    }
    if(!item_found){
        msgBox.setText("Item not found !");
        msgBox.exec();
    }
}

