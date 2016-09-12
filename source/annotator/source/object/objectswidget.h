#ifndef OBJECTSWIDGET_H
#define OBJECTSWIDGET_H

#include <AnnotatorLib/Object.h>
#include <AnnotatorLib/Session.h>
#include <QWidget>

namespace Ui {
class ObjectsWidget;
}

class ObjectsWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ObjectsWidget(QWidget *parent = 0);
  ~ObjectsWidget();
  void setSession(AnnotatorLib::Session *session);
  void reload();

protected:
  void addObject(shared_ptr<AnnotatorLib::Object> object);

 signals:
  void signal_objectSelection(shared_ptr<AnnotatorLib::Object> object);

 public slots:
  void on_refreshSession();
  void on_objectSelected(shared_ptr<AnnotatorLib::Object> object);
  void on_objectAdded(shared_ptr<AnnotatorLib::Object> object);
  void on_objectRemoved(shared_ptr<AnnotatorLib::Object> object);
 private slots:
  void on_listWidget_itemSelectionChanged();

 private:
  Ui::ObjectsWidget *ui;

  AnnotatorLib::Session *session = nullptr;
};

#endif  // OBJECTSWIDGET_H
