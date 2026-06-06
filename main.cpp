#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include "tournament_backend.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow() {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Team Name:", this);
        QLineEdit *teamEdit = new QLineEdit(this);
        QPushButton *addButton = new QPushButton("Add Team", this);
        QLabel *teamList = new QLabel(this);

        layout->addWidget(label);
        layout->addWidget(teamEdit);
        layout->addWidget(addButton);
        layout->addWidget(teamList);

        connect(addButton, &QPushButton::clicked, [=]() {
            add_team(teamEdit->text().toUtf8().constData());
            QString teams = "Teams:\n";
            for (int i = 0; i < team_count(); ++i) {
                teams += QString::fromUtf8(get_team_name(i));
                teams += "\n";
            }
            teamList->setText(teams);
        });
    }
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
