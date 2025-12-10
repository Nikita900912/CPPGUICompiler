#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QTextEdit>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>
#include <QDirIterator>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

class CompilerGUI : public QWidget {
    Q_OBJECT
public:
    CompilerGUI(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Fast /usr Compiler GUI");
        resize(1000, 700);

        QVBoxLayout *layout = new QVBoxLayout(this);

        // Кнопка выбора проекта
        selectProjectBtn = new QPushButton("Select project folder");
        layout->addWidget(selectProjectBtn);
        connect(selectProjectBtn, &QPushButton::clicked, this, &CompilerGUI::selectProject);

        // Список исходников
        sourcesView = new QListView();
        layout->addWidget(sourcesView);
        sourcesModel = new QStandardItemModel(this);
        sourcesView->setModel(sourcesModel);

        // Поиск библиотек
        searchLibs = new QLineEdit();
        searchLibs->setPlaceholderText("Search libraries...");
        layout->addWidget(searchLibs);

        // Список библиотек
        libsView = new QListView();
        layout->addWidget(libsView);
        model = new QStandardItemModel(this);
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(model);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterKeyColumn(0);
        libsView->setModel(proxyModel);

        connect(searchLibs, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterFixedString);

        // Кнопка компиляции
        compileBtn = new QPushButton("Compile");
        layout->addWidget(compileBtn);
        connect(compileBtn, &QPushButton::clicked, this, &CompilerGUI::compileProject);

        // Вывод компиляции
        output = new QTextEdit();
        layout->addWidget(output);

        // Автозагрузка библиотек
        loadLibraries("/usr");
    }

private slots:
    void selectProject() {
        QString folder = QFileDialog::getExistingDirectory(this, "Select Project Folder");
        if (folder.isEmpty()) return;

        projectPath = folder;
        sourcesModel->clear();

        QDir dir(folder);
        for (QString f : dir.entryList(QStringList() << "*.c" << "*.cpp", QDir::Files)) {
            QStandardItem *item = new QStandardItem(f);
            item->setCheckable(true);
            item->setCheckState(Qt::Checked); // по умолчанию все отмечены
            sourcesModel->appendRow(item);
        }
    }

    void compileProject() {
        if (projectPath.isEmpty()) {
            output->append("Select a project folder first!\n");
            return;
        }

        // Выбираем исходники
        QStringList sources;
        for (int i = 0; i < sourcesModel->rowCount(); ++i) {
            QStandardItem *item = sourcesModel->item(i);
            if (item->checkState() == Qt::Checked) {
                sources << projectPath + "/" + item->text();
            }
        }

        if (sources.isEmpty()) {
            output->append("No source files selected!\n");
            return;
        }

        // Формируем команду компиляции
        QStringList cmd;
        cmd << "g++" << "-o" << projectPath + "/output" << sources;

        // Добавляем библиотеки
        for (int i = 0; i < model->rowCount(); ++i) {
            QStandardItem *item = model->item(i);
            if (item->checkState() == Qt::Checked) {
                QString filePath = item->text();
                QFileInfo fi(filePath);
                QString folderPath = fi.path();
                if (filePath.endsWith(".h")) cmd << "-I" + folderPath;
                else if (filePath.endsWith(".so") || filePath.endsWith(".a")) {
                    cmd << "-L" + folderPath;
                    QString libName = fi.baseName();
                    if (libName.startsWith("lib")) libName = libName.mid(3); // убираем lib
                    cmd << "-l" + libName;
                }
            }
        }


        // Запуск компиляции
        QProcess process;
        process.start("bash", QStringList() << "-c" << cmd.join(" "));
        process.waitForFinished(-1);

        output->clear();
        output->append(process.readAllStandardOutput());
        output->append(process.readAllStandardError());

        // Подсказка при undefined reference
        QString stderrStr = process.readAllStandardError();
        for (const QString &line : stderrStr.split("\n")) {
            if (line.contains("undefined reference to")) {
                QString func = line.section('`', 1, 1);
                output->append("\n[!] Функция `" + func + "` не найдена. Возможно, забыл подключить библиотеку.\n");
            }
        }
    }

private:
    void loadLibraries(const QString &folder) {
        QDirIterator it(folder, QStringList() << "*.h" << "*.so" << "*.a", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QStandardItem *item = new QStandardItem(filePath);
            item->setCheckable(true);
            model->appendRow(item);
        }
    }

    QString projectPath;

    QPushButton *selectProjectBtn;
    QListView *sourcesView;
    QStandardItemModel *sourcesModel;

    QLineEdit *searchLibs;
    QListView *libsView;
    QStandardItemModel *model;
    QSortFilterProxyModel *proxyModel;

    QPushButton *compileBtn;
    QTextEdit *output;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    CompilerGUI w;
    w.show();
    return a.exec();
}
