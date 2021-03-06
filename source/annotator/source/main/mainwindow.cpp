// Copyright 2015-2017 Annotator Team

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>

#include <AnnotatorLib/Annotation.h>
#include <AnnotatorLib/Commands/CleanSession.h>
#include <AnnotatorLib/Commands/CompressSession.h>

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "aboutdialog.h"
#include "controller/commandcontroller.h"
#include "controller/selectioncontroller.h"
#include "gui/alert.h"
#include "gui/classesdialog.h"
#include "gui/exportannotations.h"
#include "gui/saveasdialog.h"
#include "gui/statisticsdialog.h"
#include "newprojectdialog.h"
#include "optionsdialog.h"
#include "plugins/pluginloader.h"
#include "plugins/pluginrunner.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->PlayerLayout->addWidget(&playerWidget);
  ui->annotationsLayout->addWidget(&annotationsWidget);
  ui->objectsLayout->addWidget(&objectsWidget);
  ui->selectedObjectLayout->addWidget(&selectedObject);
  ui->attributesLayout->addWidget(&attributesWidget);  // TODO
  ui->pluginsLayout->addWidget(&pluginsWidget);

  rateLabel = new QLabel;     // Frame rate
  rateLabel->setText("fps");  // Add to Status bar

  // initial locking drawing area and plugins
  enableDrawing(false);

  // make some initialization
  ui->statusBar->addPermanentWidget(rateLabel);
  playerWidget.setRateLabel(rateLabel);
  loadRecentProjects();
  connectSignalSlots();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connectSignalSlots() {
  // session
  connect(CommandController::instance(), SIGNAL(signal_refreshSession()),
          &annotationsWidget, SLOT(on_refreshSession()));
  connect(CommandController::instance(), SIGNAL(signal_refreshSession()),
          &objectsWidget, SLOT(on_refreshSession()));

  // frames
  connect(SelectionController::instance(), SIGNAL(signal_frameSelection(long)),
          &playerWidget, SLOT(on_frameSelected(long)));

  // object selection
  connect(SelectionController::instance(),
          SIGNAL(signal_objectSelection(shared_ptr<AnnotatorLib::Object>)),
          &objectsWidget,
          SLOT(on_objectSelected(shared_ptr<AnnotatorLib::Object>)));
  connect(SelectionController::instance(),
          SIGNAL(signal_objectSelection(shared_ptr<AnnotatorLib::Object>)),
          &selectedObject,
          SLOT(on_objectSelected(shared_ptr<AnnotatorLib::Object>)));
  connect(SelectionController::instance(),
          SIGNAL(signal_objectSelection(shared_ptr<AnnotatorLib::Object>)),
          &annotationsWidget,
          SLOT(on_objectSelected(shared_ptr<AnnotatorLib::Object>)));
  connect(SelectionController::instance(),
          SIGNAL(signal_objectSelection(shared_ptr<AnnotatorLib::Object>)),
          &pluginsWidget,
          SLOT(on_objectSelected(shared_ptr<AnnotatorLib::Object>)));

  // upate annotations
  connect(CommandController::instance(),
          SIGNAL(signal_newAnnotation(shared_ptr<AnnotatorLib::Annotation>)),
          &annotationsWidget,
          SLOT(on_annotationAdded(shared_ptr<AnnotatorLib::Annotation>)));
  connect(
      CommandController::instance(),
      SIGNAL(signal_removedAnnotation(shared_ptr<AnnotatorLib::Annotation>)),
      &annotationsWidget,
      SLOT(on_annotationRemoved(shared_ptr<AnnotatorLib::Annotation>)));
  // update objects
  connect(CommandController::instance(),
          SIGNAL(signal_newObject(shared_ptr<AnnotatorLib::Object>)),
          &annotationsWidget,
          SLOT(on_objectAdded(shared_ptr<AnnotatorLib::Object>)));
  connect(CommandController::instance(),
          SIGNAL(signal_removedObject(shared_ptr<AnnotatorLib::Object>)),
          &annotationsWidget,
          SLOT(on_objectRemoved(shared_ptr<AnnotatorLib::Object>)));
  connect(CommandController::instance(),
          SIGNAL(signal_newObject(shared_ptr<AnnotatorLib::Object>)),
          &objectsWidget,
          SLOT(on_objectAdded(shared_ptr<AnnotatorLib::Object>)));
  connect(CommandController::instance(),
          SIGNAL(signal_removedObject(shared_ptr<AnnotatorLib::Object>)),
          &objectsWidget,
          SLOT(on_objectRemoved(shared_ptr<AnnotatorLib::Object>)));

  // update attributes
  connect(SelectionController::instance(), SIGNAL(signal_frameSelection(long)),
          &attributesWidget, SLOT(setFrame(long)));
  connect(&playerWidget, SIGNAL(signal_frameChanged(long)), &attributesWidget,
          SLOT(setFrame(long)));

  // autoAnnotate
  connect(&pluginsWidget, SIGNAL(signal_autoAnnotate(bool)), &playerWidget,
          SLOT(on_autoAnnotate(bool)));
}

void MainWindow::setRateValue(QString value) { rateLabel->setText(value); }

void MainWindow::openProject(std::shared_ptr<AnnotatorLib::Project> project) {
  if (project) {
    this->project = project;

    this->session = project->getSession();
    CommandController::instance()->setSession(session);
    playerWidget.setProject(project);

    annotationsWidget.setSession(this->session);
    objectsWidget.setSession(this->session);
    selectedObject.setProject(project);
    attributesWidget.setSession(this->session);

    this->setRateValue("FPS: " + QString::number(playerWidget.getRateValue()));
    setWindowTitle(project);

    CommandController::instance()->doEmitRefreshSession();

    for (Annotator::Plugin *p :
         Annotator::PluginLoader::getInstance().getPlugins()) {
      p->setProject(project);
    }

    // save path to last opened project in settings
    addRecentProject(QString::fromStdString(project->getPath()));

    // lock/unlock project
    lock(!project->isActive());
  }
}

void MainWindow::loadRecentProjects() {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  foreach (QVariant v, settings.value("RecentProjects").toList()) {
    QFileInfo fileInfo(v.toString());

    QAction *recentProjectAction =
        new QAction(fileInfo.baseName(), ui->menuRecentProjects);
    recentProjectAction->setToolTip(v.toString());
    // load project when action is triggered
    connect(recentProjectAction, &QAction::triggered, this,
            [recentProjectAction, this]() {
              QString recentProject = recentProjectAction->toolTip();
              this->openProject(
                  AnnotatorLib::Project::loadPath(recentProject.toStdString()));
            });
    ui->menuRecentProjects->addAction(recentProjectAction);
  }
}

void MainWindow::addRecentProject(QString projectfile) {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());

  QVariantList recentProjects = settings.value("RecentProjects").toList();
  if (recentProjects.contains(projectfile))
    recentProjects.removeOne(projectfile);
  recentProjects.prepend(projectfile);
  settings.setValue("RecentProjects", recentProjects);
  settings.setValue("LastProjectPath", projectfile);
}

void MainWindow::on_actionClose_Project_triggered() { closeProject(); }

void MainWindow::closeProject() {
  if (!this->project) {
    return;
  }
  // lock drawing area and plugins
  enableDrawing(false);

  annotationsWidget.setSession(nullptr);
  annotationsWidget.on_refreshSession();
  objectsWidget.setSession(nullptr);
  objectsWidget.on_refreshSession();
  selectedObject.setProject(nullptr);
  attributesWidget.setSession(nullptr);

  // ask if project should be saved
  QCheckBox *cb_lock =
      new QCheckBox("Yes, lock this project. Labeling is completed now.");
  cb_lock->setChecked(!project->isActive());
  QMessageBox msgbox;
  msgbox.setParent(0);
  msgbox.setStyleSheet(
      "color: black;"
      "background-color: white;"
      "selection-color: black;"
      "selection-background-color: black;");
  msgbox.setText(tr("Save project before close?\n"));
  msgbox.setIcon(QMessageBox::Icon::Question);
  msgbox.addButton(QMessageBox::No);
  msgbox.addButton(QMessageBox::Yes);
  msgbox.setDefaultButton(QMessageBox::Yes);
  QPalette p;
  p.setColor(QPalette::WindowText, QColor("Black"));
  p.setColor(QPalette::Window, QColor("White"));
  msgbox.setPalette(p);
  msgbox.setCheckBox(cb_lock);

  if (msgbox.exec() == QMessageBox::Yes) {
    this->project->setActive(cb_lock->checkState() != Qt::Checked);
    project->save();
  }
  this->setWindowTitle(nullptr);
  this->project.reset();
  playerWidget.closeProject();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  this->closeProject();
  event->accept();
  QWidget::closeEvent(event);
}

void MainWindow::on_actionQuit_triggered() {
  if (!playerWidget.videoplayer->isStop()) {
    playerWidget.videoplayer->pauseIt();
  }
  this->close();
}

void MainWindow::setWindowTitle(
    std::shared_ptr<AnnotatorLib::Project> project) {
  if (project == nullptr) {
    QMainWindow::setWindowTitle(QApplication::applicationName() +
                                " - No project opened.");
  } else {
    QMainWindow::setWindowTitle(QApplication::applicationName() + " - " +
                                QString::fromStdString(project->getName()));
  }
}

void MainWindow::on_actionOpen_Project_triggered() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), ".",
                                                  tr("Project Files (*.xml)"));
  if (QFile::exists(fileName)) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Annotator::PluginLoader::getInstance();
    try {
      std::shared_ptr<AnnotatorLib::Project> project =
          AnnotatorLib::Project::loadPath(fileName.toStdString());
      openProject(project);
      QApplication::restoreOverrideCursor();
    } catch (std::exception &e) {
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, "Failure while loading Project",
                           QString(e.what()));
    }
  }
  CommandController::instance()->doEmitRefreshSession();
}

void MainWindow::on_actionNew_Project_triggered() {
  NewProjectDialog dialog;
  dialog.exec();
  if (dialog.getProject()) {
    this->project = dialog.getProject();
    AnnotatorLib::Project::savePath(this->project, this->project->getPath());
    openProject(this->project);
  }
}

void MainWindow::on_actionSave_Project_triggered() {
  if (this->project != nullptr) {
    project->save();
  }
}

void MainWindow::on_actionUndo_triggered() {
  if (this->project != nullptr) {
    CommandController::instance()->undo();
  }
}

void MainWindow::on_actionRedo_triggered() {
  if (this->project != nullptr) {
    CommandController::instance()->redo();
  }
}

void MainWindow::on_actionClear_Session_triggered() {
  QMessageBox::StandardButton resBtn = QMessageBox::question(
      this, "", tr("This will delete everything! Are you sure?\n"),
      QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Yes);

  if (resBtn == QMessageBox::Yes) {
    shared_ptr<AnnotatorLib::Commands::CleanSession> cmd =
        std::make_shared<AnnotatorLib::Commands::CleanSession>(this->session);
    CommandController::instance()->execute(cmd);
  }
}

void MainWindow::on_actionCompress_Session_triggered() {
  // TODO: freeze window and show progress bar
  int nmb_annotations_before = session->getAnnotations().size();
  shared_ptr<AnnotatorLib::Commands::CompressSession> cmd =
      std::make_shared<AnnotatorLib::Commands::CompressSession>(this->session);
  CommandController::instance()->execute(cmd);
  int nmb_annotations_after = session->getAnnotations().size();

  Alert msgBox;
  msgBox.setText(QString::fromStdString(
      std::string("Compression algorithm has removed ") +
      std::to_string(nmb_annotations_before - nmb_annotations_after) +
      std::string(" annotations.\n")));
  msgBox.setIcon(QMessageBox::Information);
  msgBox.setStandardButtons(0);
  msgBox.setAutoClose(true);
  msgBox.setTimeout(3);  // Closes after three seconds
  msgBox.exec();
}

void MainWindow::on_actionAbout_triggered() {
  AboutDialog aboutDialog;
  aboutDialog.exec();
}

void MainWindow::on_actionClasses_triggered() {
  if (session != nullptr) {
    ClassesDialog dialog(session, this);
    dialog.exec();
  }
}

void MainWindow::enableDrawing(bool enable) {
  this->pluginsWidget.setEnabled(enable);
  this->playerWidget.enableDrawing(enable);
}

void MainWindow::on_actionLock_project_toggled(bool b) { lock(b); }

void MainWindow::lock(bool b) {
  this->project->setActive(!b);
  enableDrawing(!b);

  if (b) {
    Alert msgBox;
    msgBox.setToolTip("Warning");
    msgBox.setText(QString::fromStdString(
        std::string("This project is locked. Unlock via the menu.")));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(0);
    msgBox.setAutoClose(true);
    msgBox.setTimeout(3);  // Closes after three seconds
    msgBox.exec();
  }
}

void MainWindow::on_actionProject_Statistics_triggered() {
  if (project) {
    StatisticsDialog dialog(project, this);
    dialog.exec();
  }
}

void MainWindow::on_actionRun_Plugins_Dialog_triggered() {
  PluginRunner pluginRunner(project, this);
  pluginRunner.exec();
}

void MainWindow::on_actionExport_Annotations_triggered() {
  ExportAnnotations dialog(project, this);
  dialog.exec();
}

void MainWindow::on_action_Options_triggered() {
  OptionsDialog dialog;
  dialog.exec();
}

void MainWindow::on_actionSave_Project_As_triggered() {
  SaveAsDialog dialog(project, this);
  dialog.exec();
}
