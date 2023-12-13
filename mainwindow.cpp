#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "qjsonobject.h"
#include "QKeyEvent"
#include <QCloseEvent>
#include "QSizePolicy"
#include "console_window.h"
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , sp(new QSerialPort(this))
    , bg_proc(new BgProcess(this))
    , lg_proc(this)
    , dmesg_proc(this)
    , m_console(new Console)
    , fw(new PushPullListForm())
{
    ui->setupUi(this);
    setupAdbTab();
    setupSerialTab();
    setupPushPullTab();
    isADBConnected = false;
    setupQuickLinksTab();
    m_console->mainWindow(this);
    serial_file_opend = false;
    fw->setMainWindowInstance(this);
    //write key strokes read from consle
    QObject::connect(m_console, &Console::getData, this, [this](const QByteArray &data){
        this->sp->write(data);
    });


    // Setup Config editor tab
    QObject::connect(ui->btn_add_adb_btn,&QPushButton::clicked,this,&MainWindow::writeConfigJson);
    QObject::connect(ui->btn_remove_adb_btn,&QPushButton::clicked,this,&MainWindow::removeAdbButton);

    QObject::connect(ui->btn_add_serial_btn,&QPushButton::clicked,this,&MainWindow::addSerialButton);
    QObject::connect(ui->btn_remove_serial_btn,&QPushButton::clicked,this,&MainWindow::removeSerialButton);

    QObject::connect(ui->btn_add_UR,&QPushButton::clicked,this,&MainWindow::addQuickLink);
    QObject::connect(ui->btn_remove_url,&QPushButton::clicked,this,&MainWindow::removeQuickLink);

    QObject::connect(ui->btn_save_repo_path,&QPushButton::clicked,this,&MainWindow::saveRepoPath);




}
MainWindow::~MainWindow()
{
    if(sp->isOpen()){
        sp->flush();
        sp->close();
    }
    if(bg_proc->isRunning()){
        bg_proc->exit();
    }

    if(lg_proc.isRunning()){
        lg_proc.exit();
    }
    if(dmesg_proc.isRunning()){
        dmesg_proc.exit();
    }
    if(pro.isOpen()){
        pro.close();
    }

    delete ui;
}


//*************************************************************************************************************
//Quick Links Tab functions
//*************************************************************************************************************
void MainWindow::setupQuickLinksTab(int update){
    if(!config_json.contains("quick_links")){
        updateCommandOutput("Quick Links info not found");
        return;
    }
    static QWidget *scrolWidget = new QWidget();
    static QGridLayout* scrolGrid = new QGridLayout();
    static int i=1;
    QJsonArray pushListArr = config_json.value("quick_links").toArray();
//    QHash<QLabel*,QStringList> url_data;
//    QHash<QLabel*,QLabel*> url_search_tags;



    foreach (QJsonValue var, pushListArr) {
        if(update){
            var = pushListArr.last();
        }
        QLabel* myLabel = new QLabel();
        QLabel* tag = new QLabel();
//        url_list.append(myLabel);
        QStringList keywords;
        keywords.append(var.toObject()["display_text"]. toString());
        keywords.append(var.toObject()["keywords"].toString().split(","));



        myLabel->setText("<a href=\""+ var.toObject()["url"].toString() + "\">" + var.toObject()["display_text"].toString()+"</a>");
        myLabel->setTextFormat(Qt::RichText);
        myLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        myLabel->setOpenExternalLinks(true);
        myLabel->setScaledContents(true);
        myLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        tag->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        scrolGrid->addWidget(myLabel,i++,0,1,3);
//        scrolGrid->addWidget(tag,i++,4,1,2);

        url_data.insert(myLabel,keywords);
//        url_search_tags.insert(myLabel,tag);
        if(update){
            break;
        }
    }

    ui->comboBox_remove_url->clear();
    foreach (auto btns, pushListArr) {
        ui->comboBox_remove_url->addItem(btns.toObject()["display_text"].toString());
    }
    scrolWidget->setLayout(scrolGrid);
    if(update){
        return;
    }


    QLineEdit* url_search = new QLineEdit();
//    if(!update){
    //    QList<QLabel*> url_list;
        scrolWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    //    scrolWidget->setScaledContents(true);
        scrolGrid->addWidget(url_search,0,0,1,4);

//    }
    url_search->setPlaceholderText("Srearch Urls");
    QObject::connect(url_search,&QLineEdit::textEdited,this,[this,url_search](){
        QString search = url_search->text();

        for (auto var : url_data.keys()) {
//            updateCommandOutput("===================================");
//            updateCommandOutput(url_data.value(var).join("\n"));
//            url_data.value(var).contains()
            for(auto str: url_data.value(var)){
//                updateCommandOutput(str + "      " + search);
                if(str.contains(search,Qt::CaseInsensitive)){
                    var->show();

//                    url_search_tags.value(var)->setText(str);
//                    url_search_tags.value(var)->show();

//                    var->setScaledContents(true);
//                    updateCommandOutput("true11111");
                    break;
                }
                else{
                    var->hide();
//                    url_search_tags.value(var)->hide();
                }
            }
        }
    });



    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(scrolWidget);
    ui->tabWidget->insertTab(3,scrollArea,"Quick Links");

}

//*************************************************************************************************************
//Serial Tab functions
//*************************************************************************************************************
void MainWindow::setupSerialTab(){

    // Connecting Serial Monitor Tab Related Objects

    // Connect serial tab preloaded buttons
    QObject::connect(ui->btn_serial_start,&QPushButton::clicked,this,&MainWindow::openSerial);
    QObject::connect(ui->btn_serial_clear_op,&QPushButton::clicked,this,&MainWindow::clearSerialOp);
    QObject::connect(ui->btn_serial_reload_comport_list,&QPushButton::clicked,this,&MainWindow::populateComPortList);
    QObject::connect(ui->btn_serial_write,&QPushButton::clicked,this,&MainWindow::writeSerialBtn);
    QObject::connect(ui->btn_serial_dmesg_n1,&QPushButton::clicked,this,&MainWindow::writeSerialBtn);
    QObject::connect(ui->btn_serial_ctrl_c_adb,&QPushButton::clicked,this,&MainWindow::writeSerialBtn);
    QObject::connect(ui->btn_serial_reboot,&QPushButton::clicked,this,&MainWindow::writeSerialBtn);
    QObject::connect(ui->btn_serial_reboot_fastboot,&QPushButton::clicked,this,&MainWindow::writeSerialBtn);

    // Connecting serial port realted functionalities
    QObject::connect(sp,&QSerialPort::readyRead,this,&MainWindow::readSerial);
    QObject::connect(sp, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error){
        if (error != QSerialPort::NoError) {
            QMessageBox::critical(this, tr("Critical Error"), this->sp->errorString());

        }
    });

    // Function to write data to serial port top combo box
    QObject::connect(ui->comboBox_serial_write->lineEdit(),&QLineEdit::returnPressed,this,[this](){
        writeSerial(this->ui->comboBox_serial_write->lineEdit()->text().trimmed());
    });

    // Function to save serial output to file
    QObject::connect(ui->checkBox_save_serial,&QCheckBox::stateChanged,this,[this](){
        if(ui->checkBox_save_serial->isChecked()){

            serial_filename = QDateTime::currentDateTime().toString("ddMMMMyyyy_hh_mm_ss_ap") + "_serial_logs.txt";
            serial_file = new QFile(serial_filename);
            serial_file_opend = serial_file->open(QIODevice::ReadWrite);
            if (serial_file_opend) {
                serial_stream = new QTextStream(serial_file);
                updateSerialOutput("Saving logs to " + serial_filename,Qt::darkGreen);
                ui->label_save_txt->setText("Saving logs to " + serial_filename);
            }
            else{
                updateSerialOutput("File error",Qt::red);
                return;
            }
        }
        else{
            if(serial_file_opend && serial_file->isOpen()){
                serial_file_opend = false;
                serial_file->close();
                ui->label_save_txt->setText("Stopped Saving");
            }
        }
    });

    m_console->setEnabled(false);
    populateComPortList();

//    QObject::connect(ui->com_port_drop_list,&QComboBox::highlighted,this,&MainWindow::populateComPortList);
    addCustomSerialBtns();
}


// This function will list all the available com ports on the system
void MainWindow::populateComPortList(){
    QList<QSerialPortInfo> sp_info = QSerialPortInfo::availablePorts();
    ui->com_port_drop_list->clear();
    foreach (auto var, sp_info) {
       ui->com_port_drop_list->addItem(var.portName() +"->" +var.description());
    }
}

// Function to add custom buttons on serial tab using config.json
void MainWindow::addCustomSerialBtns(int update){
    // hacky code to remove old serial text output and add serial console
    static int i=4;
    static int j=0;
    static int btn_count = 0;

    if(!update){
        ui->serial_op->hide(); // hide old serial text
        QGridLayout* tab_gridL = qobject_cast<QGridLayout*>(ui->serial_tab->layout()) ;
        QTextEdit* adbTxtbox = ui->serial_op;
        tab_gridL->removeWidget(adbTxtbox);
        tab_gridL->addWidget(m_console,5,0,1,5);// added console to serial tab
    }


    if(config_json.contains("serial_buttons")){


        QJsonArray serialBtnList = config_json.value("serial_buttons").toArray();
        foreach (QJsonValue var, serialBtnList) {
            if(update){
                var = serialBtnList.last();
            }
            BtnInfo bi;
            bi.cmd = var.toObject()["cmd"].toString();
            serialBtns.insert(var.toObject()["btn_txt"].toString(),bi);

            if(j>=5){
                j=j%5;
                i++;
            }

            QGridLayout* tab_gridL = qobject_cast<QGridLayout*>(ui->serial_tab->layout()) ;
            QString btnTxt = var.toObject()["btn_txt"].toString() ;
            QPushButton* btn = new QPushButton(btnTxt,ui->serial_tab);
            btn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            tab_gridL->addWidget(btn,i,j++);


            // Moving last row objects to new last row
//            QTextEdit* adbTxtbox = ui->serial_op;
//            tab_gridL->removeWidget(adbTxtbox);
            tab_gridL->removeWidget(m_console);
            tab_gridL->addWidget(m_console,i+2,0,1,5);
//            tab_gridL->addWidget(adbTxtbox,i+2,0,2,5);

            QPushButton* clr_btn = ui->btn_serial_clear_op;
            tab_gridL->removeWidget(clr_btn);
            tab_gridL->addWidget(clr_btn,i+1,4);

            QLabel* ser_lbl = ui->label_save_txt;
            tab_gridL->removeWidget(ser_lbl);
            tab_gridL->addWidget(ser_lbl,i+1,1,1,2);

            QCheckBox* ser_save_cb = ui->checkBox_save_serial;
            tab_gridL->removeWidget(ser_save_cb);
            tab_gridL->addWidget(ser_save_cb,i+1,3);

            QObject::connect(btn,&QPushButton::clicked,this,&MainWindow::serialBtnPressed);
            btn_count++;
            if(update){
                break;
            }
        }
        if(update){
            updateCommandOutput("Added new custom Serial button.");
        }
        else{
            updateCommandOutput("Added " + QString::number(btn_count) + " custom Serial buttons.");
        }
        //Add custom adb buttons to delete combobox list
        ui->comboBox_remove_serial_btn->clear();
        foreach (auto btns, serialBtnList) {
            ui->comboBox_remove_serial_btn->addItem(btns.toObject()["btn_txt"].toString());
        }
    }
    else{
        updateCommandOutput("No Serial buttons found!");
    }

}

void MainWindow::serialBtnPressed(){
    //do nothing
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
//    ui->textEdit->append(btn->text());

    if(serialBtns.contains(btn->text())){
        writeSerial(serialBtns.value(btn->text()).cmd);
    }

}

int MainWindow::getBaudRate(){
    int ret = 115200;
    int selectedBaudRate = ui->comboBox_serial_baudrate->currentIndex();
    switch(selectedBaudRate){
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    }

    return ret;
}

void MainWindow::openSerial(){
    QPushButton* senderBtn = qobject_cast<QPushButton*>(sender());
    if(sp->isOpen()){
        sp->close();
        ui->com_port_drop_list->setEnabled(true);
        updateSerialOutput("Serial Port Closed !");
        senderBtn->setText("Start Serial");
        m_console->setEnabled(false);
    }
    else{
        try {
            QString selected_com_port;

            selected_com_port = ui->com_port_drop_list->currentText().split("->")[0].trimmed();

            sp->setPortName(selected_com_port.split("->")[0].trimmed());

            sp->setBaudRate(115200);

            updateSerialOutput("Opening Serial Port [" + selected_com_port +"] ...");
            if(sp->open(QIODevice::ReadWrite)){
                updateSerialOutput("Serial Port [" + selected_com_port +"] opened");
//                sp->flush();
                m_console->setEnabled(true);
                writeSerial("");
                writeSerial("su");
            }
            else{
                updateSerialOutput("Serial Port error");
                return;
            }
        } catch (std::exception& e) {
            ui->serial_op->append(e.what());
        }
        ui->com_port_drop_list->setEnabled(false);
        senderBtn->setText("Stop Serial");
    }
}

void MainWindow::readSerial(){
//    static int bc=1;
//    if(sp !=NULL && !sp.isOpen()){
//        return ;
//    }
    QString data;
    QByteArray a;
    a = sp->readAll();
    data = a;
    if (data.contains("")){
        return;
    }
    else if(data == ""){

        ui->btn_serial_ctrl_c_adb->click();
        return;
    }
    if(serial_file_opend && serial_file->isOpen()){
        *serial_stream << data;
    }
    m_console->putData(data.toLocal8Bit());
}

void MainWindow::writeSerial(QString data){
    try {
        if(sp->isOpen()){
            data = data + "\n";
            sp->write(data.toLocal8Bit());
        }
        else{
            updateSerialOutput("Serial Port not open !!",Qt::red);
        }
    } catch (std::exception& e) {
//        ui->serial_op->append(e.what());
        updateSerialOutput(e.what(),Qt::red);
    }

}

void MainWindow::writeSerialSplChar(char c){
    if(sp->isOpen()){
        sp->write(&c);
    }
    else{
        updateSerialOutput("Serial Port not open !!",Qt::red);
    }
}

void MainWindow::writeSerialBtn(){
    QString data;
    QPushButton* senderBtn = qobject_cast<QPushButton*>(sender());

    if(senderBtn->text() == ui->btn_serial_write->text()){
        data = ui->comboBox_serial_write->lineEdit()->text().trimmed();
        data = data.trimmed();

    }
    else if(senderBtn->text() == ui->btn_serial_dmesg_n1->text()){
        data = "su \ndmesg -n1;";
    }
    else if(senderBtn->text() == ui->btn_serial_ctrl_c_adb->text()){
//        writeSerialSplChar(3);
        QKeyEvent* k = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_C, Qt::ControlModifier,"");
        m_console->keyPressEventProxy(k);
        return;
    }
    else if(senderBtn->text() == ui->btn_serial_reboot->text()){
        data = "reboot";
    }
    else if(senderBtn->text() == ui->btn_serial_reboot_fastboot->text()){
        data = "reboot fastboot";
    }
    else{
        updateSerialOutput("Invalid Request");
    }
    writeSerial(data);
}

void MainWindow::updateSerialOutput(QString data,QColor color,bool newLine){
    QTextCursor cursor(ui->serial_op->textCursor());
    cursor.movePosition(QTextCursor::End);
    ui->serial_op->setTextCursor(cursor);
    ui->serial_op->setTextColor(color);
    if(newLine){
        ui->serial_op->insertPlainText("\n" + data + "\n");
    }
    else{
        ui->serial_op->insertPlainText(data);
    }
    ui->serial_op->verticalScrollBar()->setValue(ui->serial_op->verticalScrollBar()->maximum());

}

void MainWindow::clearSerialOp(){
   m_console->clear();
   ui->btn_serial_ctrl_c_adb->click();
}

//*************************************************************************************************************



//*************************************************************************************************************
//ADB Command Tab functions
//*************************************************************************************************************
bool MainWindow::isADBDeviceFound(){
    return isADBConnected;
}

void MainWindow::setupAdbTab(){
    //Adding Info about Preloaded buttons
    adbBtns.insert(ui->btn_adb_clear_dmesg->text(),BtnInfo{"dmesg -C","adb",true,false});
    adbBtns.insert(ui->btn_adb_clear_logcat->text(),BtnInfo{"logcat -c","adb",true,false});
    adbBtns.insert(ui->btn_adb_dmesg->text(),BtnInfo{"dmesg -wT","adb",true,true});
    adbBtns.insert(ui->btn_adb_logcat->text(),BtnInfo{"logcat","adb",true,true});//////
    adbBtns.insert(ui->btn_adb_reboot->text(),BtnInfo{"reboot","adb",false,false});
    adbBtns.insert(ui->btn_adb_remount->text(),BtnInfo{"remount","adb",false,false});
    adbBtns.insert(ui->btn_adb_root->text(),BtnInfo{"root","adb",false,false});
    adbBtns.insert(ui->btn_adb_shell->text(),BtnInfo{"","adb",true,true});

    //Conecting Preloaded buttons
    QObject::connect(ui->btn_adb_clear_dmesg,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_clear_logcat,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_dmesg,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_logcat,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_reboot,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_remount,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_root,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_shell,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
    QObject::connect(ui->btn_adb_clear_output,&QPushButton::clicked,this,
                     [this](){
        this->ui->textEdit->setText("");
        this->ui->txt_push_pull->setText("");
    });

    QObject::connect(ui->btn_cmd_prompt,&QPushButton::clicked,this,[this](){
//        this->ui->comboBox_cmd_prompt->lineEdit()->setFocus();
//        QKeyEvent *key_press = new QKeyEvent(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
        QApplication::sendEvent(this->ui->comboBox_cmd_prompt, new QKeyEvent(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier) );
    });

    QObject::connect(ui->comboBox_cmd_prompt->lineEdit(),&QLineEdit::returnPressed,this,[this](){
       if(ui->comboBox_cmd_prompt->lineEdit()->text().trimmed() != "" ){
           system("start cmd /k " + ui->comboBox_cmd_prompt->lineEdit()->text().trimmed().toLocal8Bit());
       }
    });



    //Capture Enter(return) event in QCombobox and simulate button press
    QObject::connect(ui->comboBox_logcat_grep->lineEdit(),&QLineEdit::returnPressed,this,[this](){
        this->updateCommandOutput(this->ui->comboBox_logcat_grep->lineEdit()->text());
        this->ui->btn_adb_logcat->click();
    });

    QObject::connect(ui->comboBox_dmesg_grep->lineEdit(),&QLineEdit::returnPressed,this,[this](){
        this->updateCommandOutput(this->ui->comboBox_dmesg_grep->lineEdit()->text());
        this->ui->btn_adb_dmesg->click();
    });

    QObject::connect(ui->checkBox_save_logcat,&QCheckBox::stateChanged,this,[this](){
        if(!ui->checkBox_save_logcat->isChecked()){
            lg_proc.stopProc();
        }
        else{
            updateCommandOutput("Press Launch logcat start saving logs / uncheck to stop saving.");
        }
    });

    QObject::connect(ui->checkBox_save_dmesg,&QCheckBox::stateChanged,this,[this](){
        if(!ui->checkBox_save_dmesg->isChecked()){
            dmesg_proc.stopProc();
        }
        else{
            updateCommandOutput("Press Launch dmesg start saving logs / uncheck to stop saving.");
        }
    });

    QObject::connect(ui->btn_adb_screencap,&QPushButton::clicked,this,[this](){
        QString filename = "screencapture_" + QDateTime::currentDateTime().toString("ddMMMMyyyy_hh_mm_ss_ap") + ".png";
        QString cmd = "adb exec-out screencap -p > " + filename ; // Prep command to start cmd and add adb prefix
        updateCommandOutput("CMD RUN-> "+ cmd);
        system(cmd.toLocal8Bit());
    });

    // Triggered when "pro" has standard output ready for reading.
    QObject::connect(&pro, &QProcess::readyReadStandardOutput,this,&MainWindow::readProcOp);
    QObject::connect(&pro, &QProcess::readyReadStandardError,this,&MainWindow::readProcErr);

    //Adding custom ADB buttons
    if(readConfigJson()){
        updateCommandOutput("Config file loaded.");
        addCustomAdbBtns();
    }
    else{
        updateCommandOutput("Config file not loaded");
    }

    bg_proc->start();
}

void MainWindow::addCustomAdbBtns(int update){
    static int i=4;
    static int j=0;
    static int btn_count = 0;
    QGridLayout* tab_gridL = qobject_cast<QGridLayout*>(ui->adb_cmds->layout()) ;

    if(update){
        QJsonArray adbBtnListArr = config_json.value("adb_buttons").toArray();
        auto var = adbBtnListArr.last() ;
        BtnInfo bi;
        bi.cmd = var.toObject()["cmd"].toString();
        bi.app = var.toObject()["app"].toString();
        bi.extWindow = var.toObject()["extWindow"].toBool();
        bi.shell_cmd = var.toObject()["shell_cmd"].toBool();

        adbBtns.insert(var.toObject()["btn_txt"].toString(),bi);

        if(j>=5){
            j=j%5;
            i++;
        }

//        QGridLayout* tab_gridL = qobject_cast<QGridLayout*>(ui->adb_cmds->layout()) ;
        QString btnTxt = var.toObject()["btn_txt"].toString() ;
        QPushButton* btn = new QPushButton(btnTxt,ui->tabWidget);
        btn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        tab_gridL->addWidget(btn,i,j++);

        QObject::connect(btn,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
        btn_count++;

        /*relocate fixed widgets*/
        QTextEdit* adbTxtbox = ui->textEdit;
        tab_gridL->removeWidget(adbTxtbox);
        tab_gridL->addWidget(adbTxtbox,i+2,0,2,5);

        QPushButton* clr_btn = ui->btn_adb_clear_output;
        tab_gridL->removeWidget(clr_btn);
        tab_gridL->addWidget(clr_btn,i+1,4);

        // reposition run cmd propmpt button
        tab_gridL->removeWidget(ui->btn_cmd_prompt);
        tab_gridL->addWidget(ui->btn_cmd_prompt,i+1,2);

        // reposition cmd prompt Combobox
        tab_gridL->removeWidget(ui->comboBox_cmd_prompt);
        tab_gridL->addWidget(ui->comboBox_cmd_prompt,i+1,0,1,2);

        //Add list of custom adb buttons to delete combobox list
        ui->comboBox_adbbtn_list->clear();
        foreach (auto btns, adbBtnListArr) {
            ui->comboBox_adbbtn_list->addItem(btns.toObject()["btn_txt"].toString());
        }

        return;
    }

    // Adding the buttons when app is started
    if(config_json.contains("adb_buttons")){
            QJsonArray adbBtnListArr = config_json.value("adb_buttons").toArray();
            foreach (auto var, adbBtnListArr) {

                BtnInfo bi;
                bi.cmd = var.toObject()["cmd"].toString();
                bi.app = var.toObject()["app"].toString();
                bi.extWindow = var.toObject()["extWindow"].toBool();
                bi.shell_cmd = var.toObject()["shell_cmd"].toBool();

                adbBtns.insert(var.toObject()["btn_txt"].toString(),bi);

                if(j>=5){
                    j=j%5;
                    i++;
                }


                QString btnTxt = var.toObject()["btn_txt"].toString() ;
                QPushButton* btn = new QPushButton(btnTxt,ui->tabWidget);
                btn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
                tab_gridL->addWidget(btn,i,j++);

                QObject::connect(btn,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
                btn_count++;
            }

            /*relocate fixed widgets*/
            QTextEdit* adbTxtbox = ui->textEdit;
            tab_gridL->removeWidget(adbTxtbox);
            tab_gridL->addWidget(adbTxtbox,i+2,0,2,5);

            QPushButton* clr_btn = ui->btn_adb_clear_output;
            tab_gridL->removeWidget(clr_btn);
            tab_gridL->addWidget(clr_btn,i+1,4);

            // reposition run cmd propmpt button
            tab_gridL->removeWidget(ui->btn_cmd_prompt);
            tab_gridL->addWidget(ui->btn_cmd_prompt,i+1,2);

            // reposition cmd prompt Combobox
            tab_gridL->removeWidget(ui->comboBox_cmd_prompt);
            tab_gridL->addWidget(ui->comboBox_cmd_prompt,i+1,0,1,2);


            updateCommandOutput("Added " + QString::number(btn_count) + " custom ADB buttons.");

            //Add custom adb buttons to delete combobox list
            foreach (auto btns, adbBtnListArr) {
                ui->comboBox_adbbtn_list->addItem(btns.toObject()["btn_txt"].toString());
            }
        }
    else{
        updateCommandOutput("No ADB buttons found!");
    }
}

//to be deleted
void MainWindow::addButtons(){
    static int i=1;
    static int j=0;

    if(j>=5){
        j=j%5;
        i++;
    }
    ui->textEdit->append("i="+ QString::number(i)+" j="+QString::number(j) );

    QGridLayout* tab_gridL = qobject_cast<QGridLayout*>(ui->adb_cmds->layout()) ;
    QPushButton* senderBtn = qobject_cast<QPushButton*>(sender());

    QString btnTxt = senderBtn->text()+"ABCD" ;
    QPushButton* btn = new QPushButton(btnTxt,ui->tabWidget);
    btn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    tab_gridL->addWidget(btn,i,j++);
    QTextEdit* adbTxtbox = ui->textEdit;
    tab_gridL->removeWidget(adbTxtbox);
    tab_gridL->addWidget(adbTxtbox,i+1,0,2,5);
    QObject::connect(btn,&QPushButton::clicked,this,&MainWindow::adbBtnPressed);
}

void MainWindow::adbBtnPressed(){
    QPushButton* btn = qobject_cast<QPushButton*>(sender());

    if(adbBtns.contains(btn->text())){


        if(adbBtns.value(btn->text()).app == "adb"){
            runAdbCommand(btn->text());
        }
        else if(adbBtns.value(btn->text()).app == "cmd"){
            runCmdCommand(btn->text());
        }
    }

}

void MainWindow::runAdbCommand(QString name){
   QString cmd;
   BtnInfo curr_btn;
   QString logcat_dmesg_proc_cmd;

   if(adbBtns.contains(name)){ //Check if btn info exits
       curr_btn = adbBtns.value(name);
       if(curr_btn.extWindow){ //If cmd has to be run in ext window
           cmd = "start cmd /k adb "; // Prep command to start cmd and add adb prefix
           if(curr_btn.shell_cmd){
               if(name!=ui->btn_adb_shell->text()){
                   cmd += "exec-out "; //If command has to be run in adb shell add "shell" prefix
               }
               else{
                   cmd += "shell ";
               }
           }
           if(name==ui->btn_adb_logcat->text() && ui->comboBox_logcat_grep->lineEdit()->text().trimmed()!="")
           {
               cmd += "\"logcat | grep -iE '" +  ui->comboBox_logcat_grep->lineEdit()->text().trimmed()+"'\"";
               logcat_dmesg_proc_cmd += "logcat | grep -iE '" +  ui->comboBox_logcat_grep->lineEdit()->text().trimmed()+"'";
           }
           else if(name==ui->btn_adb_dmesg->text() && ui->comboBox_dmesg_grep->lineEdit()->text().trimmed()!=""){
               cmd += "\"dmesg -wT | grep -iE '" +  ui->comboBox_dmesg_grep->lineEdit()->text().trimmed()+"'\"";
               logcat_dmesg_proc_cmd += "dmesg -wT | grep -iE '" +  ui->comboBox_dmesg_grep->lineEdit()->text().trimmed()+"'";
           }
           else{
               cmd += curr_btn.cmd; // Adding cmd to be run.
               logcat_dmesg_proc_cmd += curr_btn.cmd;
           }

           if(name==ui->btn_adb_logcat->text() && ui->checkBox_save_logcat->isChecked()){
               lg_proc.setCmd(logcat_dmesg_proc_cmd);
               lg_proc.start();
           }
           else if(name==ui->btn_adb_dmesg->text() && ui->checkBox_save_dmesg->isChecked()){
               dmesg_proc.setCmd(logcat_dmesg_proc_cmd);
               dmesg_proc.start();
           }
           updateCommandOutput("ADB RUN -> " + cmd);
//           system(QString("start cmd /k " + adbBtns.value(name).cmd).toLocal8Bit());
           system(cmd.toLocal8Bit());
       }
       else{
           QString exePath = "adb.exe";
           cmd += "adb ";
           QStringList cmd_list;
           if(curr_btn.shell_cmd){
               cmd_list<<"exec-out";
           }
           cmd_list<<curr_btn.cmd;
           if(pro.isOpen()){
               pro.close();
           }
           cmd += cmd_list.join(" ");
           updateCommandOutput("ADB RUN -> " + cmd);
           pro.start(exePath,cmd_list);
//           pro.start(exePath,QStringList()<<"/c"<<);
//           pro.waitForStarted();
//           pro.write(adbBtns.value(name).cmd.toLocal8Bit());
//           pro.waitForBytesWritten();
//           pro.closeWriteChannel();
//           updateCommandOutput(pro.readAll());

       }
   }
}

//==========================================================
void MainWindow::LogcatProcess::run(){
//    main_window->updateCommandOutput(QDateTime::currentDateTime().toString("ddMMMMyyyy_h_m_s_ap"));

    QString filename = QDateTime::currentDateTime().toString("ddMMMMyyyy_hh_mm_ss_ap") + "_logcat_" +
            main_window->ui->comboBox_logcat_grep->lineEdit()->text().trimmed().split("|").join("_") + ".txt";
    file = new QFile(filename);
    if (file->open(QIODevice::ReadWrite)) {
        stream = new QTextStream(file);
        main_window->updateCommandOutput("Saving logs to " + filename,Qt::darkGreen);
    }
    else{
        main_window->updateCommandOutput("File error",Qt::red);
        return;
    }

    QObject::connect(&pro, &QProcess::readyReadStandardOutput,this,&LogcatProcess::readProcOp);
    QObject::connect(&pro, &QProcess::readyReadStandardError,this,&LogcatProcess::readProcErr);
    QStringList lis;
    lis<<"shell";
    lis<<cmd;
    pro.start("adb.exe",lis);
    while(pro.isOpen()){}
    if(file->isOpen()){
        file->close();
    }
    main_window->updateCommandOutput("Stopped logcat log captue ! ",Qt::red);
}

void MainWindow::LogcatProcess::readProcOp(){
    QString op = pro.readAllStandardOutput();
//    main_window->updateCommandOutput(op,Qt::darkGreen,false);
    if(file->isOpen()){
        *stream << op ;
    }
}

void MainWindow::LogcatProcess::readProcErr(){
    QString op = pro.readAllStandardError();
//    main_window->updateCommandOutput(pro.readAllStandardError(),Qt::red,false);
    if(file->isOpen()){
        *stream << op ;
    }
}
//==========================================================
void MainWindow::DmesgProcess::run(){

    QString filename = QDateTime::currentDateTime().toString("ddMMMMyyyy_hh_mm_ss_ap") + "_dmesg_" +
            main_window->ui->comboBox_dmesg_grep->lineEdit()->text().trimmed().split("|").join("_") + ".txt";
    file = new QFile(filename);
    if (file->open(QIODevice::ReadWrite)) {
        stream = new QTextStream(file);
        main_window->updateCommandOutput("Saving logs to " + filename,Qt::darkGreen);
    }
    else{
        main_window->updateCommandOutput("File error",Qt::red);
        return;
    }
    QObject::connect(&pro, &QProcess::readyReadStandardOutput,this,&DmesgProcess::readProcOp);
    QObject::connect(&pro, &QProcess::readyReadStandardError,this,&DmesgProcess::readProcErr);
    QStringList lis;
    lis<<"shell";
    lis<<cmd;

    pro.start("adb.exe",lis);
    while(pro.isOpen()){}
    if(file->isOpen()){
        file->close();
    }
    main_window->updateCommandOutput("Stopped dmesg log captue ! ",Qt::red);
}

void MainWindow::DmesgProcess::readProcOp(){
    QString op = pro.readAllStandardOutput();
    if(file->isOpen()){
        *stream << op ;
    }
}

void MainWindow::DmesgProcess::readProcErr(){
    QString op = pro.readAllStandardError();
    if(file->isOpen()){
        *stream << op ;
    }
}
//==========================================================


void MainWindow::runCmdCommand(QString name,bool internal){
    QString cmd;
    BtnInfo curr_btn;
    if(adbBtns.contains(name)){
        curr_btn = adbBtns.value(name);
    }else if(internal){

    }
    else{
        return;
    }


    if(curr_btn.extWindow){
        cmd += "start cmd /k " + curr_btn.cmd; // Prep command to start cmd and add adb prefix
        system(cmd.toLocal8Bit());
    }
    else{
        QString exePath = "cmd.exe";
        cmd += "cmd ";
        QStringList cmd_list;
//        if(curr_btn.shell_cmd){
//            cmd_list<<"exec-out";
//        }
        cmd_list<<"/k"<<curr_btn.cmd;
        if(pro.isOpen()){
            pro.close();
        }
        cmd += cmd_list.join(" ");
        updateCommandOutput("CMD RUN -> " + cmd);
        pro.start(exePath,cmd_list);
    }

}

void MainWindow::readProcOp(){
    updateCommandOutput(pro.readAllStandardOutput(),Qt::darkGreen,false);
}

void MainWindow::readProcErr(){
    updateCommandOutput(pro.readAllStandardError(),Qt::red,false);
}

void MainWindow::updateCommandOutput(QString data,QColor color,bool newLine){

    QTextCursor cursor(ui->txt_push_pull->textCursor());
    cursor.movePosition(QTextCursor::End);
    ui->txt_push_pull->setTextCursor(cursor);

    cursor=ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);

    ui->textEdit->setTextColor(color);
    ui->txt_push_pull->setTextColor(color);

    if(newLine){
        ui->textEdit->insertPlainText(data + "\n");
        ui->txt_push_pull->insertPlainText(data + "\n");
    }
    else{
        ui->textEdit->insertPlainText(data);
        ui->txt_push_pull->insertPlainText(data);
    }

    ui->textEdit->verticalScrollBar()->setValue(ui->textEdit->verticalScrollBar()->maximum());
    ui->txt_push_pull->verticalScrollBar()->setValue(ui->txt_push_pull->verticalScrollBar()->maximum());
}

void MainWindow::BgProcess::run(){
    bool isDeviceFound = false;
    QObject::connect(&pro, &QProcess::readyReadStandardOutput,this,&BgProcess::readProcOp);
    while(true){
        pro.start("adb.exe",QStringList()<<"devices");
        pro.waitForFinished();
        pro.close();
        QStringList op_s = op.trimmed().split("\n");
        if(op_s.length()>1){
            if(isDeviceFound){

            }
            else{
                isDeviceFound = true;
                main_window->isADBConnected = true;
                main_window->ui->lineEdit->setStyleSheet("color:green;");
                main_window->ui->lineEdit->setText(op_s[1].split("\t")[0]);
                main_window->ui->btn_adb_root->click();
//                main_window->ui->btn_adb_remount->click();
                main_window->ui->push_pull_tab->setEnabled(true);
//                main_window->writeSerial("su");
            }
        }
        else{
            isDeviceFound = false;
            main_window->isADBConnected = false;
            main_window->ui->lineEdit->setStyleSheet("color:red;");
            main_window->ui->lineEdit->setText("No Device Found");
//            main_window->ui->push_pull_tab->setEnabled(false);
        }
//        for (auto var : op_s) {
//            main_window->updateCommandOutput("DEBUG--->" + var);
//        }
//        main_window->updateCommandOutput(op);



        sleep(2);
    }
}

void MainWindow::BgProcess::readProcOp(){
    op = pro.readAllStandardOutput();
//    main_window->updateCommandOutput(pro.readAllStandardOutput(),Qt::blue);

}

//*************************************************************************************************************
//ADB Push/Pull Command Tab functions
//*************************************************************************************************************

void MainWindow::setupPushPullTab(){

    QObject::connect(ui->btn_push,&QPushButton::clicked,this,&MainWindow::pushBtnPressed);
    QObject::connect(ui->btn_pull,&QPushButton::clicked,this,&MainWindow::pullBtnPressed);
    QObject::connect(ui->btn_adb_clear_2,&QPushButton::clicked,this,[this] (){
        this->ui->btn_adb_clear_output->click();
    });
    QObject::connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](){
        if(this->ui->tabWidget->currentIndex()==2){
            this->ui->btn_adb_root->click();
            this->ui->btn_adb_remount->click();
        }
    });
    QObject::connect(ui->radiobtn_localdir,&QRadioButton::toggled,this,[this](){
        if(this->ui->radiobtn_localdir->isChecked()){
            this->ui->line_push_path->clear();
            this->ui->line_push_path->setPlaceholderText("Enter local dir path to push binary");
        }
    });
    QObject::connect(ui->radiobtn_repo,&QRadioButton::toggled,this,[this](){
        if(this->ui->radiobtn_repo->isChecked()){
            if(repo_path==""){
                this->ui->line_push_path->clear();
                this->ui->line_push_path->setPlaceholderText("Enter local dir path to push binary");
            }
            else{
                this->ui->line_push_path->setText(repo_path);
            }
        }
    });

    QObject::connect(ui->btn_edit_push_pull_list,&QPushButton::clicked,this,[this] (){
        fw->show();
    });

    if(config_json.contains("repo_path")){
        repo_path = config_json.value("repo_path").toString();
    }
    if(repo_path==""){
       ui->radiobtn_localdir->setChecked(true);
    }
    else{
        ui->radiobtn_repo->setChecked(true);
    }

//    updateCommandOutput(repo_path);
    fillPushPullList();
}

void MainWindow::fillPushPullList(int update){
    QVBoxLayout *pushPullBoxLayout = qobject_cast<QVBoxLayout*>(ui->push_pull_list->layout());

    if(config_json.contains("push_list")){
        QJsonArray pushListArr = config_json.value("push_list").toArray();

        try {
            foreach (QJsonValue var, pushListArr) {
                PushPullInfo ppi;
                if(update){
                    var = pushListArr.last();
                }
                QCheckBox *cb = new QCheckBox(tr(var.toObject()["name"].toString().toLocal8Bit()));
                ppi.cb = cb;
                ppi.repo_path = var.toObject()["repo_path"].toString();
                ppi.push_path = var.toObject()["push_path"].toString();
                pushPullBoxLayout->addWidget(cb);
                pushPullCheckBoxList.insert(var.toObject()["name"].toString(),ppi);
                if(update){
                    return;
                }
            }
        } catch (std::exception& e) {
//            ui->serial_op->append(e.what());
            updateCommandOutput(e.what());
        }
    }
    else{
        updateCommandOutput("No push_list ");
    }
}

void MainWindow::pushBtnPressed(){

    if(!isADBDeviceFound()){
        updateCommandOutput("PUSH FAILED: ADB device not connected",Qt::red);
        return;
    }

    PushProc *process;

    bool repo_dir = ui->radiobtn_repo->isChecked();
    QString pushPath ;


    pushPath = ui->line_push_path->text();

    for (auto varkey : pushPullCheckBoxList.keys()) {
        auto var = pushPullCheckBoxList.value(varkey);
        QStringList cmd_list;
        cmd_list<<"push";

        if(var.cb->isChecked()){
            QString final_path;
            if(repo_dir){
                final_path = pushPath + var.repo_path + varkey;
            }
            else{
                final_path = pushPath + varkey ;
            }

            cmd_list<<final_path;
            cmd_list<<var.push_path;
            updateCommandOutput("ADB RUN -> "+cmd_list.join(" "));
            process = new PushProc(this,cmd_list);
            process->run();//
        }
    }
}

void MainWindow::pullBtnPressed(){
    if(!isADBDeviceFound()){
        updateCommandOutput("PULL FAILED: ADB device not connected",Qt::red);
        return;
    }

    PushProc *process;

    for (auto varkey : pushPullCheckBoxList.keys()) {
        auto var = pushPullCheckBoxList.value(varkey);
        QStringList cmd_list;
        cmd_list<<"pull";

        if(var.cb->isChecked()){

            cmd_list<<var.push_path+varkey;
            updateCommandOutput("ADB RUN -> "+cmd_list.join(" "));

            process = new PushProc(this,cmd_list);
            process->run();
        }
    }
}

void MainWindow::PushProc::run(){
    QObject::connect(&pro, &QProcess::readyReadStandardOutput,this,&PushProc::readProcOp);
    QObject::connect(&pro, &QProcess::readyReadStandardError,this,&PushProc::readProcErr);


    pro.start("adb.exe",cmd);
}

void MainWindow::PushProc::readProcOp(){
    QString data = pro.readAllStandardOutput();
    main_window->updateCommandOutput(data,Qt::darkGreen,false);
    if(data.contains("1 file pushed")){

       main_window->updateCommandOutput("ADB RUN -> adb shell sync");
       if(pro.isOpen()){
           pro.close();
       }
       pro.start("adb.exe", QStringList()<<"exec-out"<<"sync");
    }
}

void MainWindow::PushProc::readProcErr(){
    main_window->updateCommandOutput(pro.readAllStandardError(),Qt::red,false);
}

bool MainWindow::readConfigJson(){
    bool result = false;
    try {
        QFile file;
        QString val;
        file.setFileName("test.json");
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
            val = file.readAll();
            file.close();
            QJsonDocument jd = QJsonDocument::fromJson(val.toUtf8());
            config_json = jd.object();
            result = true;
        }
    } catch (std::exception e) {
        updateCommandOutput(e.what());
    }
    return result;
}

bool MainWindow::updateJson(QJsonDocument doc){
    bool ret = false;

    try {
        QFile file;
        file.setFileName("test.json");
        if(file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)){
            file.write(doc.toJson());
            file.close();
            ret = true;
        }
    } catch (std::exception e) {
        QMessageBox msgBox;
        msgBox.setText(e.what());
        msgBox.exec();
    }
    return ret;
}

void MainWindow::writeConfigJson(){


    QString app;
    QString btn_txt;
    QString btn_cmd;
    bool extWindow;
    bool shell = false;

    QMessageBox msgBox;


    if(ui->lineEdit_adb_btn_cmd->text().isEmpty() || ui->lineEdit_adb_btn_txt->text().isEmpty()){
        msgBox.setText("Please enter all data correctly");
        msgBox.exec();
        return;
    }
    app = ui->comboBox_appType_adb->currentText();
    extWindow = ui->checkBox_adb_btn_ext_win->isChecked();
    btn_txt = ui->lineEdit_adb_btn_txt->text();
    if(ui->lineEdit_adb_btn_cmd->text().contains("adb shell") ){
        btn_cmd = ui->lineEdit_adb_btn_cmd->text().remove("adb shell ");
        shell = true;
    }
    else if( ui->lineEdit_adb_btn_cmd->text().contains("adb exec-out")){
        btn_cmd = ui->lineEdit_adb_btn_cmd->text().remove("adb exec-out ");
        shell = true;
    }
    else{
        btn_cmd = ui->lineEdit_adb_btn_cmd->text().remove("adb ");
    }


    auto adb_btn_data = QJsonObject({
                                        qMakePair("app",app),
                                        qMakePair("btn_txt",btn_txt),
                                        qMakePair("cmd",btn_cmd),
                                        qMakePair("extWindow",extWindow),
                                        qMakePair("shell_cmd",shell)
                                    });

    QJsonArray arr = config_json.value("adb_buttons").toArray();
    arr.append(adb_btn_data);
    config_json["adb_buttons"] = arr;


    QJsonDocument doc(config_json);
    if(updateJson(doc)){
        addCustomAdbBtns(1);
        msgBox.setText("new adb Button added");
        msgBox.exec();
        ui->comboBox_appType_adb->setCurrentIndex(0);
        ui->lineEdit_adb_btn_txt->setText("");
        ui->checkBox_adb_btn_ext_win->setCheckState(Qt::Unchecked);
        ui->lineEdit_adb_btn_cmd->setText("");
    }
    else{
        msgBox.setText("Failed!! Could not add adb button");
        msgBox.exec();
    }
}

void MainWindow::removeAdbButton(){
    QString btn_txt = ui->comboBox_adbbtn_list->currentText();
    QJsonArray arr = config_json.value("adb_buttons").toArray();
    int index = 0;
    bool btn_found = false;

    QMessageBox msgBox;
    for(auto val: arr){
        if(val.toObject()["btn_txt"].toString() == btn_txt){
            btn_found = true;
            updateCommandOutput("btn found");
            arr.removeAt(index);
            config_json["adb_buttons"] = arr;
            QJsonDocument doc(config_json);
            if(updateJson(doc)){
                msgBox.setText("Adb Button removed.");
                msgBox.exec();
                ui->comboBox_adbbtn_list->clear();
                foreach (auto btns, arr) {
                    ui->comboBox_adbbtn_list->addItem(btns.toObject()["btn_txt"].toString());
                }
            }
            else{
                msgBox.setText("Failed!! Could not remove adb button");
                msgBox.exec();
            }
            break;
        }
        index++;
    }
    if(!btn_found){
        msgBox.setText("Btn not found !");
        msgBox.exec();
    }
}

void MainWindow::addSerialButton(){
    QString btn_txt = ui->lineEdit_serial_btn_add_txt->text().trimmed();
    QString btn_cmd = ui->lineEdit_serial_btn_add_cmd->text().trimmed();
     QMessageBox msgBox;

    if(btn_txt.isEmpty() || btn_cmd.isEmpty()){
        msgBox.setText("Please enter all data correctly");
        msgBox.exec();
        return;
    }
    else{
        auto serial_btn_data = QJsonObject({
                                            qMakePair("btn_txt",btn_txt),
                                            qMakePair("cmd",btn_cmd)
                                        });

        QJsonArray arr = config_json.value("serial_buttons").toArray();
        arr.append(serial_btn_data);
        config_json["serial_buttons"] = arr;

        QJsonDocument doc(config_json);
        if(updateJson(doc)){
            addCustomSerialBtns(1);
            msgBox.setText("new serial button added");
            msgBox.exec();
            ui->lineEdit_serial_btn_add_txt->clear();
            ui->lineEdit_serial_btn_add_cmd->clear();
        }
        else{
            msgBox.setText("Failed!! Could not add serial button");
            msgBox.exec();
        }
    }
}

void MainWindow::removeSerialButton(){
    QString btn_txt = ui->comboBox_remove_serial_btn->currentText();
    QJsonArray arr = config_json.value("serial_buttons").toArray();
    int index = 0;
    bool btn_found = false;

    QMessageBox msgBox;
    for(auto val: arr){
        if(val.toObject()["btn_txt"].toString() == btn_txt){
            btn_found = true;
            updateCommandOutput("btn found");
            arr.removeAt(index);
            config_json["serial_buttons"] = arr;
            QJsonDocument doc(config_json);
            if(updateJson(doc)){
                msgBox.setText("serial button removed.");
                msgBox.exec();
                ui->comboBox_remove_serial_btn->clear();
                foreach (auto btns, arr) {
                    ui->comboBox_remove_serial_btn->addItem(btns.toObject()["btn_txt"].toString());
                }
            }
            else{
                msgBox.setText("Failed!! Could not remove serial button");
                msgBox.exec();
            }
            break;
        }
        index++;
    }
    if(!btn_found){
        msgBox.setText("Btn not found !");
        msgBox.exec();
    }
}

void MainWindow::addQuickLink(){
    QString url_txt = ui->lineEdit_add_URL_txt->text().trimmed();
    QString url_cmd = ui->lineEdit_add_URL_link->text().trimmed();
    QMessageBox msgBox;

    if(url_txt.isEmpty() || url_cmd.isEmpty()){
        msgBox.setText("Please enter all data correctly");
        msgBox.exec();
        return;
    }
    else{
        auto url_data = QJsonObject({

                                         qMakePair("display_text",url_txt),
                                         qMakePair("url",url_cmd)
                                     });

        QJsonArray arr = config_json.value("quick_links").toArray();
        arr.append(url_data);
        config_json["quick_links"] = arr;

        QJsonDocument doc(config_json);
        if(updateJson(doc)){
            setupQuickLinksTab(1);
            msgBox.setText("new url added");
            msgBox.exec();
            ui->lineEdit_add_URL_txt->clear();
            ui->lineEdit_add_URL_link->clear();
        }
        else{
            msgBox.setText("Failed!! Could not add url");
            msgBox.exec();
        }
    }
}

void MainWindow::removeQuickLink(){
    QString url_txt = ui->comboBox_remove_url->currentText();
    QJsonArray arr = config_json.value("quick_links").toArray();
    int index = 0;
    bool url_found = false;

    QMessageBox msgBox;
    for(auto val: arr){
        if(val.toObject()["display_text"].toString() == url_txt){
            url_found = true;
//            updateCommandOutput("btn found");
            arr.removeAt(index);
            config_json["quick_links"] = arr;
            QJsonDocument doc(config_json);
            if(updateJson(doc)){
                msgBox.setText("URL removed.");
                msgBox.exec();
                ui->comboBox_remove_url->clear();
                foreach (auto btns, arr) {
                    ui->comboBox_remove_url->addItem(btns.toObject()["display_text"].toString());
                }
            }
            else{
                msgBox.setText("Failed!! Could not remove URL");
                msgBox.exec();
            }
            break;
        }
        index++;
    }
    if(!url_found){
        msgBox.setText("Btn not found !");
        msgBox.exec();
    }
}

void MainWindow::saveRepoPath(){
    QString repo_path = ui->line_push_path->text().trimmed();
//    updateCommandOutput("repo path from lsdsd--> "+repo_path);
    QMessageBox msgBox;
    if(repo_path.isEmpty()){
        msgBox.setText("Cannot save empty repo path !");
        msgBox.exec();
        return;
    }
    config_json["repo_path"] = QJsonValue(repo_path);

//    updateCommandOutput(config_json.value("repo_path").toString());
    QJsonDocument doc(config_json);
    if(updateJson(doc)){
//        setupQuickLinksTab(1);
        msgBox.setText("new repo path saved");
        msgBox.exec();
    }
    else{
        msgBox.setText("Failed!! Could not save repo path");
        msgBox.exec();
    }
}
