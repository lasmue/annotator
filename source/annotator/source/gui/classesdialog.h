#ifndef CLASSESDIALOG_H
#define CLASSESDIALOG_H

#include <AnnotatorLib/Session.h>
#include <QDialog>


namespace Ui {
class ClassesDialog;
}

class ClassesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClassesDialog(AnnotatorLib::Session *session, QWidget *parent = 0);
    ~ClassesDialog();

private slots:
    void on_closeButton_clicked();

    void on_addNewButton_clicked();

    void on_newLineEdit_returnPressed();

protected:
    /**
     * @brief reloadClasses Clears classes list and recreates its content.
     */
    void reloadClasses();
    AnnotatorLib::Session * session;

private:
    Ui::ClassesDialog *ui;
};

#endif // CLASSESDIALOG_H
