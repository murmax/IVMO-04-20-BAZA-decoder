#include "mainwindow.h"
#include "ui_mainwindow.h"



QMap<QString,QString> MainWindow::iniParams;
QMap<QString,QString> MainWindow::iniParamsTranslation;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug()<<"QTextCodec::availableCodecs():"<<QTextCodec::availableCodecs();

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    loadParamsFromIni();

    if (iniParams.value("UsePostgreSQL")=="1")
    {
        bool ok;
        dbase = QSqlDatabase::addDatabase("QPSQL");
        dbase.setDatabaseName(iniParams.value("PostgreSQL_DBName")/*"myDB1"*/);
        dbase.setPort(iniParams.value("PostgreSQL_Port").toInt(&ok)/*5432*/);
        dbase.setHostName(iniParams.value("PostgreSQL_HostName")/*"localhost"*/);
        dbase.setUserName(iniParams.value("PostgreSQL_UserName")/*"User1"*/);
        dbase.setPassword(iniParams.value("PostgreSQL_UserPassword")/*"123"*/);
    } else
    {
        dbase = QSqlDatabase::addDatabase("QSQLITE");
        dbase.setDatabaseName("local.sqlite");
    }
    ui->btn_check->setVisible(false);
    if (!syncWithDB())
    {
        ui->btn_parse->setEnabled(false);
        ui->te_dbError->append(dbase.lastError().text());
    } else
    {
        ui->te_dbError->append("БД подключена корректно\n");
    }


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btn_browse_clicked()
{
    QString fileNames = QFileDialog::getOpenFileNames(this,"",QDir::currentPath(),"*.csv").join(";");
    ui->le_browse->setText(fileNames);
}



//INI
void MainWindow::loadParamsFromIni()
{
    QFile fileIni("config.ini");
    if (fileIni.open(QIODevice::ReadOnly))
    {
        while (!fileIni.atEnd())
        {
            QString rawLine = ((QString)fileIni.readLine()).remove(' ').remove('\n').remove('\r').remove('\t');
            int commentPosition = rawLine.indexOf('#');
            if (commentPosition!=-1)
            {
                rawLine.remove(commentPosition,rawLine.length()-commentPosition);
            }
            QStringList line = rawLine.split('=');
            if (line.length()==2)
            {
                iniParams.insert(line[0],line[1]);
            } else if (!line.isEmpty())
            {
                qDebug()<<"[ERROR] Incorrect line in .ini file";
            }
        }
        fileIni.close();
    }
    if (!checkIniParams())
    {
        restoreIniDefaults();
        //qDebug()<<"Restoring defaults";
    }
    loadParamsTranslation();
}

void MainWindow::loadParamsTranslation()
{
    QFile file(":/Translations/iniParams.txt");
    if (file.open(QIODevice::ReadOnly))
    {
        while (!file.atEnd())
        {

            QStringList strList=((QString)file.readLine()).remove('\n').split(';');
            //qDebug()<<"strList()="<<strList.length();
            if (strList.length()==2)
            {
                iniParamsTranslation.insert(strList.at(0),strList.at(1));
            }

        }
        file.close();
    }
    //qDebug()<<"iniParamsTranslation.size()="<<iniParamsTranslation.size();
}

void MainWindow::setDefaultIniFile()
{
    QFile fileIni("config.ini");
    fileIni.remove();
    QFile fileIniNew("config.ini");
    QList<QPair<QString,QString>> pairList = getIniDefaults();

    if (fileIniNew.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&fileIniNew);
        QPair<QString,QString> pair;
        for(int i=0;i<pairList.length();i++)
        {
            pair = pairList.at(i);
            stream<<pair.first<<" = "<<pair.second<<endl;
            iniParams.insert(pair.first,pair.second);
        }

        fileIniNew.close();
    }
}

void MainWindow::restoreIniDefaults()
{
    setDefaultIniFile();

    dbase.setDatabaseName(iniParams.value("PostgreSQL_DBName")/*"myDB1"*/);
    dbase.setPort(iniParams.value("PostgreSQL_Port").toInt()/*5432*/);
    dbase.setHostName(iniParams.value("PostgreSQL_HostName")/*"localhost"*/);
    dbase.setUserName(iniParams.value("PostgreSQL_UserName")/*"User1"*/);
    dbase.setPassword(iniParams.value("PostgreSQL_UserPassword")/*"123"*/);
}

QList<QPair<QString, QString> > MainWindow::getIniDefaults()
{
   return QList<QPair<QString, QString> >
        {
            QPair<QString,QString>("PostgreSQL_DBName","myDB1"),
            QPair<QString,QString>("PostgreSQL_Port","5432"),
            QPair<QString,QString>("PostgreSQL_HostName","localhost"),
            QPair<QString,QString>("PostgreSQL_UserName","User1"),
            QPair<QString,QString>("PostgreSQL_UserPassword","123"),
            QPair<QString,QString>("UsePostgreSQL","1"),
        };
}

bool MainWindow::checkIniParams()
{
    if (iniParams.isEmpty()) return false;
    QList<QPair<QString, QString> > listParams = getIniDefaults();
    QList<QPair<QString, QString> > addList;
    for (auto e : listParams)
    {
        if (!iniParams.contains(e.first))
        {
            iniParams.insert(e.first,e.second);
            addList.append(e);
        }
    }
    if (addList.length()>0){
        QFile fileIni("config.ini");
        if (fileIni.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            for (auto pair : addList){
                    QTextStream stream(&fileIni);
                    stream<<pair.first<<" = "<<pair.second<<endl;
            }
            fileIni.close();
        }
    }

    return true;
}
//~INI

//SQL

bool MainWindow::syncWithDB()
{
    if (!dbase.open()) {
        qDebug() << "Что-то пошло не так!"<<dbase.lastError().text();
        //return -1;
        return false;
    }

    QSqlQuery query(dbase);
    QString str;
    bool b;
    bool done=false;
    int iteration=0;
    while  (!done)
    {
        switch (iteration) {
        case 0:
            str =      "CREATE TABLE IF NOT EXISTS is_table("
                       "    id SERIAL PRIMARY KEY,"
                       "    is_name VARCHAR UNIQUE);";
            break;
        case 1:
            str =      "CREATE TABLE IF NOT EXISTS bank_table("
                       "    id SERIAL PRIMARY KEY,"
                       "    bank_name VARCHAR UNIQUE);";
            break;
        case 2:
            str =      "CREATE TABLE IF NOT EXISTS crtarget_table("
                       "    id SERIAL PRIMARY KEY,"
                       "    crtarget_name VARCHAR UNIQUE);";
            break;
        case 3:
            str =      "CREATE TABLE IF NOT EXISTS crtype_table("
                       "    id SERIAL PRIMARY KEY,"
                       "    crtype_name VARCHAR UNIQUE);";
            break;
        case 4:
            str =      "CREATE TABLE IF NOT EXISTS cur_table("
                       "    id SERIAL PRIMARY KEY,"
                       "    cur_name VARCHAR UNIQUE);";
            break;


        case 5:
            str =     "CREATE TABLE IF NOT EXISTS company_table("
                      "    id SERIAL PRIMARY KEY,"
                      "    company_name VARCHAR,"
                      "    is_name integer,"
                      "    inn_name VARCHAR UNIQUE,"
                      "    CONSTRAINT company_table_is_fkey FOREIGN KEY (is_name)"
                      "        REFERENCES is_table (id) MATCH SIMPLE"
                      "        ON UPDATE NO ACTION"
                      "        ON DELETE NO ACTION);";
            break;
        case 6:
            str =     "CREATE TABLE IF NOT EXISTS rsbu_table( "
                      "    id SERIAL PRIMARY KEY,"
                      "rsbu_type VARCHAR, "
                      "rsbu_year integer, "
                      "rsbu_value integer, "
                      "company integer, "
                      "CONSTRAINT \"rsbu_table_Company_fkey\" FOREIGN KEY (company) "
                      "REFERENCES company_table (id) MATCH SIMPLE "
                      "ON UPDATE NO ACTION "
                      "ON DELETE NO ACTION);";
            break;
        case 7:
            str =     "CREATE TABLE IF NOT EXISTS cr_table("
                      "    id SERIAL PRIMARY KEY,"
                      "    company integer,"
                      "    bank integer,"
                      "    crtype integer,"
                      "    cur integer,"
                      "    crtarget integer,"
                      "    crvalue double precision,"
                      "    CONSTRAINT cr_table_bank_fkey FOREIGN KEY (bank)"
                      "        REFERENCES bank_table (id) MATCH SIMPLE"
                      "        ON UPDATE NO ACTION"
                      "        ON DELETE NO ACTION,"
                      "    CONSTRAINT cr_table_crtarget_fkey FOREIGN KEY (crtarget)"
                      "        REFERENCES crtarget_table (id) MATCH SIMPLE"
                      "        ON UPDATE NO ACTION"
                      "        ON DELETE NO ACTION,"
                      "    CONSTRAINT cr_table_crtype_fkey FOREIGN KEY (crtype)"
                      "        REFERENCES crtype_table (id) MATCH SIMPLE"
                      "        ON UPDATE NO ACTION"
                      "        ON DELETE NO ACTION,"
                      "    CONSTRAINT cr_table_cur_fkey FOREIGN KEY (cur)"
                      "        REFERENCES cur_table (id) MATCH SIMPLE"
                      "        ON UPDATE NO ACTION"
                      "        ON DELETE NO ACTION,"
                      "CONSTRAINT cr_table_company_fkey FOREIGN KEY (company)"
                      "      REFERENCES company_table (id) MATCH SIMPLE"
                      "      ON UPDATE NO ACTION"
                      "      ON DELETE NO ACTION);";
            break;
        default:
            done=true;
            str="";
            break;
        }
        if (str!=""){
            b =  query.exec(str);
        }
        if (!b){
            qDebug()<<QString("Error(")+QString::number(iteration)+"): "<<query.lastError().text();
            return false;
        }
        iteration++;

    }






/*


    if (!a_query.exec("SELECT * FROM IS_table")) {
        qDebug() << "Даже селект не получается, я пас."<<a_query.lastError().text();
        return false;
    }
    QSqlRecord rec = a_query.record();

*/


    dbase.close();
    return true;
}


//~SQL



void MainWindow::on_btn_check_clicked()
{
    if (!syncWithDB()){

        ui->te_dbError->append(dbase.lastError().text());
    } else
    {
        ui->te_dbError->append("БД подключена корректно\n");
    }

}

void MainWindow::on_btn_parse_clicked()
{
    rowsAppliedCorrect=0;
    rowsAppliedIncorrect=0;
    QStringList fileNames = ui->le_browse->text().split(',');
    for (auto fileName : fileNames){
        QFile file (fileName);
        if (file.open(QFile::ReadOnly))
        {
            ui->te_dbError->append("Файл загружен\n");
            auto allText = file.readAll();
            //QTextCodec::setCodecForLocale(QTextCodec::codecForName("cp866")); windows-1251
            QTextDecoder *decoder;
            switch (ui->cb_codec->currentIndex()) {
            case 0:
                decoder = new QTextDecoder(QTextCodec::codecForName("windows-1251"));
                break;
            case 1:
                decoder = new QTextDecoder(QTextCodec::codecForName("UTF-8"));
                break;
            case 2:
                decoder = new QTextDecoder(QTextCodec::codecForName("IBM866"));
                break;
            default:
                decoder = new QTextDecoder(QTextCodec::codecForName("windows-1251"));
                break;
            }
            //QTextDecoder decoder(QTextCodec::codecForUtfText(allText));
            QStringList rows = decoder->toUnicode(allText).split("\r\n");
            QStringList insertCommands;
            qDebug()<<"rows: "<<rows;
            if (rows.length()>2)
            {

                if (!dbase.open()) {
                    qDebug() << "Что-то пошло не так!"<<dbase.lastError().text();
                    return;
                }

                QSqlQuery query(dbase);
                firstLineHeader = rows.at(0).split(';');
                secondLineHeader = rows.at(1).split(';');


                for (int i=2;i<rows.length();i++)
                {
                    insertCommands.append(parseRow(rows.at(i),query));
                }
                for (auto com : insertCommands)
                {
                    if (com=="")
                    {
                        rowsAppliedIncorrect++;
                    } else
                    {
                        if (!query.exec(com))
                        {
                            qDebug()<<"Error final inserts:"<<query.lastError();
                            rowsAppliedIncorrect++;
                        } else
                        {
                            rowsAppliedCorrect++;
                        }
                    }
                }

                dbase.close();
            }
        } else
        {
            ui->te_dbError->append("Файл не удалось загрузить\n");
        }

        firstLineHeader.clear();
        secondLineHeader.clear();
        ui->te_dbError->append("Корректно загружено:"+QString::number(rowsAppliedCorrect)+"\n");
        ui->te_dbError->append("Некорректно загружено:"+QString::number(rowsAppliedIncorrect)+"\n");
    }
}

QStringList MainWindow::parseRowTypeFirst(QString row, QSqlQuery query)
{
    QStringList out;
    QStringList args = row.split(';');
    if (args.length()<8)
    {
        out.append("");
        return out;
    }
    for (int i=0;i<args.length();i++) {
        QString &arg = args[i];
        if (arg[0]=="\"" && arg[arg.length()-1]=="\"" && arg.length()>1)
        {
            arg.chop(1);
            arg.remove(0,1);
        }
        arg.replace("\"\"","\"");
    }
    //qDebug()<<"args after prepare:"<<args;

    int is=selectUnique("is_table","is_name",args[2],query);
    //int inn=selectUnique("inn_table","inn_name",args[1],query);
    int bank=selectUnique("bank_table","bank_name",args[4],query);
    int crtype=selectUnique("crtype_table","crtype_name",args[3],query);
    int cur=selectUnique("cur_table","cur_name",args[6],query);
    int crtarget=selectUnique("crtarget_table","crtarget_name",args[5],query);
    QMap<QString,QString> companyMap;
    companyMap.insert("is_name",QString::number(is));
    companyMap.insert("inn_name","'"+args[1]+"'");
    int company = selectUnique("company_table","company_name",args[0],query,companyMap);
    //qDebug()<<"inn"<<inn<<"is"<<is<<"bank"<<bank<<"crtype"<<crtype<<"cur"<<cur<<"crtarget"<<crtarget<<"company"<<company;
    if (company!=-1 &&
            bank!=-1 &&
            crtype!=-1 &&
            cur!=-1 &&
            crtarget!=-1){
        qDebug()<<"args7:"<<args[7]<<" new:"<<args[7].remove(' ').replace(',','.').toDouble()<<" toStr:"<<QString::number(args[7].remove(' ').toDouble());
        out.append("INSERT INTO cr_table("
                    " company, bank, crtype, cur, crtarget, crvalue)"
                    "VALUES (" +
                QString::number( company)+"," +
                QString::number( bank)+"," +
                QString::number( crtype)+"," +
                QString::number( cur)+"," +
                QString::number( crtarget)+"," +
                QString::number(args[7].remove(' ').replace(',','.').toDouble())+ ");");
    } else
    {
    }

    return out;
}

QStringList MainWindow::parseRowTypeSecond(QString row, QSqlQuery query)
{
    QStringList out;
    QStringList args = row.split(';');
    if (args.length()<6)
    {
        out.append("");
        return out;
    }
    for (int i=0;i<args.length();i++) {
        QString &arg = args[i];
        if (arg[0]=="\"" && arg[arg.length()-1]=="\"" && arg.length()>1)
        {
            arg.chop(1);
            arg.remove(0,1);
        }
        arg.replace("\"\"","\"");
    }
    //qDebug()<<"args after prepare:"<<args;

    int is=selectUnique("is_table","is_name",args[1],query);
    QMap<QString,QString> companyMap;
    companyMap.insert("is_name",QString::number(is));
    companyMap.insert("inn_name",args[3]);
    //int inn=selectUnique("inn_table","inn_name",args[3],query);
    int company = selectUnique("company_table","company_name",args[2],query,companyMap);


    int virychkaFrom = findClosestInHeader(true,"Выручка",-1);
    int incomeFrom = findClosestInHeader(true,"Чистая прибыль",-1);
    int virychkaTo = virychkaFrom;
    int incomeTo = incomeFrom;
    if (virychkaFrom!=-1){
        for (int i=virychkaFrom+1;i<secondLineHeader.length() && i<firstLineHeader.length();i++)
        {
            if (firstLineHeader.at(i)!="" || secondLineHeader.at(i)=="") break;
            virychkaTo++;
        }


        for (int i=virychkaFrom;i<=virychkaTo;i++)
        {
            int year = secondLineHeader.at(i).split(' ').value(0,"-1").toInt();
            QString type = "'Выручка'";
            if (
                    is!=-1 &&
                    company!=-1 &&
                    year!=-1){
                out.append("INSERT INTO rsbu_table("
                            "rsbu_type, rsbu_year, rsbu_value, company)"
                            "VALUES (" +
                        type +"," +
                        QString::number( year)+"," +
                        QString::number( args[i].remove(' ').replace(',','.').toDouble())+"," +
                        QString::number( company) + ");");
            }
        }
    }

    if (incomeFrom!=-1){
        for (int i=incomeFrom+1;i<secondLineHeader.length() && i<firstLineHeader.length();i++)
        {
            if (firstLineHeader.at(i)!="" || secondLineHeader.at(i)=="") break;
            incomeTo++;
        }
        for (int i=incomeFrom;i<=incomeTo;i++)
        {
            int year = secondLineHeader.at(i).split(' ').value(0,"-1").toInt();
            QString type = "'Чистая прибыль'";
            if (
                    is!=-1 &&
                    company!=-1 &&
                    year!=-1){
                out.append("INSERT INTO rsbu_table("
                            "rsbu_type, rsbu_year, rsbu_value, company)"
                            "VALUES (" +
                        type +"," +
                        QString::number( year)+"," +
                        QString::number( args[i].remove(' ').replace(',','.').toDouble())+"," +
                        QString::number( company) + ");");
            }
        }
    }






    return out;
}

int MainWindow::selectUnique(QString tableName, QString paramName, QString value, QSqlQuery query, QMap<QString, QString> additionalParams)
{
    static bool recursive = false;
    //qDebug()<<"selectUnique"<<tableName<<"value"<<value<<" recursive:"<<recursive;
    int result=-1;
    QString str;
    if (!query.exec("SELECT id FROM "+tableName+" WHERE "+paramName +" = '"+value+"';")) {
        qDebug() << "Даже селект не получается, я пас." << query.lastError();
        return -1;
    }
    QSqlRecord rec = query.record();
    if (query.next()){
        result = query.value(rec.indexOf("id")).toInt();
    } else if (!recursive)
    {
        if (additionalParams.isEmpty()){
            if (!query.exec("INSERT INTO "+tableName+" ("+paramName+") VALUES ('"+value+"');"))
            {
                qDebug() << "Error Insert Unique:" << query.lastError();
                return -1;
            }
        } else
        {
            QString names ="INSERT INTO "+tableName+" ("+paramName;
            QString values = "VALUES ('"+value+"'";
            for (auto iter = additionalParams.begin();iter!=additionalParams.end();iter++)
            {
                names+=","+iter.key();
                values+=","+iter.value();
            }
            names+=") ";
            values+=");";
            if (!query.exec(names+values))
            {
                qDebug() << "Error Insert Unique (names+values):" << query.lastError();
                return -1;
            }
        }
        recursive=true;
        result = selectUnique(tableName,paramName,value,query);
        recursive = false;
    } else
    {
        qDebug()<<"Ошибка рекурсии";
    }
    //qDebug()<<"return result"<<result;
    return result;
}

int MainWindow::findClosestInHeader(bool firstHeader, QString value, int defaultValue, int startAt, int endAt)
{
    if (firstHeader)
    {
        for (int i=startAt;i<firstLineHeader.length() && (endAt==-1 || i<=endAt);i++)
        {
            if (firstLineHeader.at(i).contains(value))
            {
                return i;
            }
            qDebug()<<"Cant find val="<<value<<" in firstLineHeader:"<<firstLineHeader;
        }
    } else
    {
        for (int i=startAt;i<firstLineHeader.length() && (endAt==-1 || i<=endAt);i++)
        {
            if (secondLineHeader.at(i).contains(value))
            {
                return i;
            }
        }
        qDebug()<<"Cant find val="<<value<<" in secondLineHeader:"<<secondLineHeader;
    }
    return defaultValue;
}



QStringList MainWindow::parseRow(QString row, QSqlQuery query)
{
    if (ui->cb_parseType->currentIndex()==0)
    {
        return parseRowTypeFirst(row,query);
    } else if (ui->cb_parseType->currentIndex()==1)
    {

        return parseRowTypeSecond(row,query);
    }
    return QStringList();
}
































