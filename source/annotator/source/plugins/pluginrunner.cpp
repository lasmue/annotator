#include "pluginrunner.h"
#include "pluginloader.h"
#include "ui_pluginrunner.h"

#include "object/objectitem.h"

#include <opencv2/imgproc/imgproc.hpp>

PluginRunner::PluginRunner(std::shared_ptr<AnnotatorLib::Project> project,
                           QWidget *parent)
    : QDialog(parent), ui(new Ui::PluginRunner) {
  this->project = project;
  ui->setupUi(this);
  initPluginsList();
  initObjectsList();

  ui->startFrameSpinBox->setMaximum(project->getImageSet()->size());
  ui->endFrameSpinBox->setMaximum(project->getImageSet()->size());
  ui->endFrameSpinBox->setValue(project->getImageSet()->size());
  ui->objectsProgressBar->hide();
  ui->framesProgressBar->hide();
}

PluginRunner::~PluginRunner() { delete ui; }

QPixmap PluginRunner::getImgCrop(long frame, int size) {
  long prev_pos = project->getImageSet()->getPosition();
  cv::Mat tmp = project->getImageSet()->getImage(frame);
  project->getImageSet()->gotoPosition(prev_pos);

  tmp.convertTo(tmp, CV_8U);
  cv::cvtColor(tmp, tmp, CV_BGR2RGB);

  QImage img((const unsigned char *)(tmp.data), tmp.cols, tmp.rows, tmp.step,
             QImage::Format_RGB888);

  QPixmap pim = QPixmap::fromImage(img);
  pim = pim.scaledToHeight(size);
  return pim;
}

void PluginRunner::on_startFrameSpinBox_valueChanged(int arg1) {
  ui->startFrameImageLabel->setPixmap(getImgCrop(arg1, 128));
}

void PluginRunner::on_endFrameSpinBox_valueChanged(int arg1) {
  ui->endFrameImageLabel->setPixmap(getImgCrop(arg1, 128));
}

void PluginRunner::initPluginsList() {
  for (Annotator::Plugin *plugin :
       Annotator::PluginLoader::getInstance().getPlugins()) {
    ui->pluginsListWidget->addItem(plugin->getName());
  }
}

void PluginRunner::initObjectsList() {
  objectIdToRowMap.clear();
  objectRowToIdMap.clear();
  if (project->getSession().get()) {
    ui->objectsListWidget->clear();
    for (auto &pair : project->getSession()->getObjects()) {
      addObject(pair.second);
    }
  }
}

void PluginRunner::addObject(shared_ptr<AnnotatorLib::Object> object) {
  QListWidgetItem *item = new QListWidgetItem(ui->objectsListWidget);

  ObjectItem *objectItem = new ObjectItem(object);
  item->setSizeHint(objectItem->minimumSizeHint());
  ui->objectsListWidget->setItemWidget(item, objectItem);
  objectIdToRowMap[object->getId()] = ui->objectsListWidget->count() - 1;
  objectRowToIdMap[ui->objectsListWidget->count() - 1] = object->getId();
}

void PluginRunner::calculate(shared_ptr<AnnotatorLib::Object> object,
                             Annotator::Plugin *plugin, int start, int end) {
  for (int i = start; i <= end; ++i) {
      ui->framesProgressBar->setValue(i);
      plugin->calculate(object, project->getSession()->getFrame(i), true);
  }
}

void PluginRunner::on_startButton_clicked() {
  if (ui->pluginsListWidget->currentRow() == -1)
    return;
  Annotator::Plugin *plugin =
      Annotator::PluginLoader::getInstance().getPlugins().at(
          ui->pluginsListWidget->currentRow());

  ui->objectsProgressBar->show();
  ui->objectsProgressBar->setMaximum(objectRowToIdMap.size());
  ui->objectsProgressBar->setValue(0);

  ui->framesProgressBar->show();
  ui->framesProgressBar->setMaximum(ui->endFrameSpinBox->value());
  ui->framesProgressBar->setValue(0);

  if (plugin) {
    int start = ui->startFrameSpinBox->value();
    int end = ui->endFrameSpinBox->value();

    QItemSelectionModel *select = ui->objectsListWidget->selectionModel();
    if (select->hasSelection()) {
      int row = select->selectedRows().front().row();
      ui->objectsProgressBar->setValue(row);
      unsigned long obj_id = objectRowToIdMap[row];
      shared_ptr<AnnotatorLib::Object> obj =
          project->getSession()->getObject(obj_id);
      calculate(obj, plugin, start, end);
    }
  }

  ui->objectsProgressBar->hide();
  ui->framesProgressBar->hide();
}

void PluginRunner::on_selectAllObjectsButton_clicked() {
  ui->objectsListWidget->selectAll();
}

void PluginRunner::on_unselectAllObjectsButton_clicked() {
  for (QListWidgetItem *item : ui->objectsListWidget->selectedItems()) {
    item->setSelected(false);
  }
}
