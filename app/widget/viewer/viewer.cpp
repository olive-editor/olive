/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "viewer.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QtMath>
#include <QVBoxLayout>

#include "audio/audiomanager.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "render/pixelformat.h"
#include "widget/menu/menu.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

ViewerWidget::ViewerWidget(QWidget *parent) :
  TimeBasedWidget(false, true, parent),
  playback_speed_(0),
  frame_cache_job_time_(0),
  color_menu_enabled_(true),
  divider_(Config::Current()["DefaultViewerDivider"].toInt()),
  override_color_manager_(nullptr),
  time_changed_from_timer_(false),
  playback_is_audio_only_(false)
{
  // Set up main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Set up stacked widget to allow switching away from the viewer widget
  stack_ = new QStackedWidget();
  layout->addWidget(stack_);

  // Create main OpenGL-based view and sizer
  sizer_ = new ViewerSizer();
  stack_->addWidget(sizer_);

  display_widget_ = new ViewerDisplayWidget();
  connect(display_widget_, &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);
  connect(display_widget_, &ViewerDisplayWidget::CursorColor, this, &ViewerWidget::CursorColor);
  connect(display_widget_, &ViewerDisplayWidget::ColorProcessorChanged, this, &ViewerWidget::ColorProcessorChanged);
  connect(display_widget_, &ViewerDisplayWidget::ColorManagerChanged, this, &ViewerWidget::ColorManagerChanged);
  connect(sizer_, &ViewerSizer::RequestMatrix, display_widget_, &ViewerDisplayWidget::SetMatrix);
  sizer_->SetWidget(display_widget_);

  // Create waveform view when audio is connected and video isn't
  waveform_view_ = new AudioWaveformView();
  stack_->addWidget(waveform_view_);

  // Create time ruler
  layout->addWidget(ruler());

  // Create scrollbar
  layout->addWidget(scrollbar());
  connect(scrollbar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
  connect(scrollbar(), &QScrollBar::valueChanged, waveform_view_, &AudioWaveformView::SetScroll);

  // Create lower controls
  controls_ = new PlaybackControls();
  controls_->SetTimecodeEnabled(true);
  controls_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  connect(controls_, &PlaybackControls::PlayClicked, this, static_cast<void(ViewerWidget::*)()>(&ViewerWidget::Play));
  connect(controls_, &PlaybackControls::PauseClicked, this, &ViewerWidget::Pause);
  connect(controls_, &PlaybackControls::PrevFrameClicked, this, &ViewerWidget::PrevFrame);
  connect(controls_, &PlaybackControls::NextFrameClicked, this, &ViewerWidget::NextFrame);
  connect(controls_, &PlaybackControls::BeginClicked, this, &ViewerWidget::GoToStart);
  connect(controls_, &PlaybackControls::EndClicked, this, &ViewerWidget::GoToEnd);
  connect(controls_, &PlaybackControls::TimeChanged, this, &ViewerWidget::SetTimeAndSignal);
  layout->addWidget(controls_);

  // FIXME: Magic number
  SetScale(48.0);

  // Start background renderers
  renderer_ = new OpenGLBackend(this);

  connect(waveform_view_, &AudioWaveformView::TimeChanged, this, &ViewerWidget::SetTimeAndSignal);

  connect(PixelFormat::instance(), &PixelFormat::FormatChanged, this, &ViewerWidget::UpdateRendererParameters);

  SetAutoMaxScrollBar(true);
}

ViewerWidget::~ViewerWidget()
{
  QList<ViewerWindow*> windows = windows_;

  foreach (ViewerWindow* window, windows) {
    delete window;
  }
}

void ViewerWidget::TimeChangedEvent(const int64_t &i)
{
  if (!time_changed_from_timer_) {
    Pause();
  }

  controls_->SetTime(i);
  waveform_view_->SetTime(i);

  if (GetConnectedNode() && last_time_ != i) {
    rational time_set = Timecode::timestamp_to_time(i, timebase());

    UpdateTextureFromNode(time_set);

    PushScrubbedAudio();

    display_widget_->SetTime(time_set);
  }

  last_time_ = i;
}

void ViewerWidget::ConnectNodeInternal(ViewerOutput *n)
{
  if (!n->video_params().time_base().isNull()) {
    SetTimebase(n->video_params().time_base());
  } else if (n->audio_params().sample_rate() > 0) {
    SetTimebase(n->audio_params().time_base());
  } else {
    SetTimebase(rational());
  }

  connect(n, &ViewerOutput::TimebaseChanged, this, &ViewerWidget::SetTimebase);
  connect(n, &ViewerOutput::SizeChanged, this, &ViewerWidget::SizeChangedSlot);
  connect(n, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);
  connect(n, &ViewerOutput::ParamsChanged, this, &ViewerWidget::UpdateRendererParameters);
  connect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedRange);
  connect(n, &ViewerOutput::GraphChangedFrom, this, &ViewerWidget::UpdateStack);

  ruler()->SetPlaybackCache(n->video_frame_cache());

  n->audio_playback_cache()->SetParameters(AudioRenderingParams(n->audio_params(),
                                                                SampleFormat::kInternalFormat));

  SizeChangedSlot(n->video_params().width(), n->video_params().height());
  LengthChangedSlot(n->GetLength());

  ColorManager* using_manager;
  if (override_color_manager_) {
    using_manager = override_color_manager_;
  } else if (n->parent()) {
    using_manager = static_cast<Sequence*>(n->parent())->project()->color_manager();
  } else {
    qWarning() << "Failed to find a suitable color manager for the connected viewer node";
    using_manager = nullptr;
  }

  display_widget_->ConnectColorManager(using_manager);
  foreach (ViewerWindow* window, windows_) {
    window->display_widget()->ConnectColorManager(using_manager);
  }

  divider_ = CalculateDivider();

  UpdateRendererParameters();

  UpdateStack();

  if (GetConnectedTimelinePoints()) {
    waveform_view_->SetViewer(GetConnectedNode()->audio_playback_cache());
    waveform_view_->ConnectTimelinePoints(GetConnectedTimelinePoints());
  }

  // Set texture to new texture (or null if no viewer node is available)
  ForceUpdate();
}

void ViewerWidget::DisconnectNodeInternal(ViewerOutput *n)
{
  Pause();

  SetTimebase(rational());

  disconnect(n, &ViewerOutput::TimebaseChanged, this, &ViewerWidget::SetTimebase);
  disconnect(n, &ViewerOutput::SizeChanged, this, &ViewerWidget::SizeChangedSlot);
  disconnect(n, &ViewerOutput::LengthChanged, this, &ViewerWidget::LengthChangedSlot);
  disconnect(n, &ViewerOutput::ParamsChanged, this, &ViewerWidget::UpdateRendererParameters);
  disconnect(n->video_frame_cache(), &FrameHashCache::Invalidated, this, &ViewerWidget::ViewerInvalidatedRange);
  disconnect(n, &ViewerOutput::GraphChangedFrom, this, &ViewerWidget::UpdateStack);

  ruler()->SetPlaybackCache(nullptr);

  // Effectively disables the viewer and clears the state
  SizeChangedSlot(0, 0);

  display_widget_->DisconnectColorManager();
  foreach (ViewerWindow* window, windows_) {
    window->display_widget()->DisconnectColorManager();
  }

  waveform_view_->SetViewer(nullptr);
  waveform_view_->ConnectTimelinePoints(nullptr);
}

void ViewerWidget::ConnectedNodeChanged(ViewerOutput *n)
{
  renderer_->SetViewerNode(n);
}

void ViewerWidget::ScaleChangedEvent(const double &s)
{
  TimeBasedWidget::ScaleChangedEvent(s);

  waveform_view_->SetScale(s);
}

void ViewerWidget::resizeEvent(QResizeEvent *event)
{
  TimeBasedWidget::resizeEvent(event);

  /*
  int new_div = CalculateDivider();
  if (new_div != divider_) {
    divider_ = new_div;

    UpdateRendererParameters();
  }
  */

  UpdateMinimumScale();
}

ViewerDisplayWidget *ViewerWidget::display_widget() const
{
  return display_widget_;
}

void ViewerWidget::TogglePlayPause()
{
  if (IsPlaying()) {
    Pause();
  } else {
    Play();
  }
}

bool ViewerWidget::IsPlaying() const
{
  return playback_speed_ != 0;
}

void ViewerWidget::ConnectViewerNode(ViewerOutput *node, ColorManager* color_manager)
{
  override_color_manager_ = color_manager;

  TimeBasedWidget::ConnectViewerNode(node);
}

void ViewerWidget::SetColorMenuEnabled(bool enabled)
{
  color_menu_enabled_ = enabled;
}

void ViewerWidget::SetOverrideSize(int width, int height)
{
  SizeChangedSlot(width, height);
}

void ViewerWidget::SetMatrix(const QMatrix4x4 &mat)
{
  display_widget_->SetMatrix(mat);
  foreach (ViewerWindow* vw, windows_) {
    vw->display_widget()->SetMatrix(mat);
  }
}

void ViewerWidget::SetFullScreen(QScreen *screen)
{
  if (!screen) {
    // Try to find the screen that contains the mouse cursor currently
    foreach (QScreen* test, QGuiApplication::screens()) {
      if (test->geometry().contains(QCursor::pos())) {
        screen = test;
        break;
      }
    }

    // Fallback, just use the first screen
    if (!screen) {
      screen = QGuiApplication::screens().first();
    }
  }

  ViewerWindow* vw = new ViewerWindow(this);

  vw->setGeometry(screen->geometry());
  vw->showFullScreen();
  vw->display_widget()->ConnectColorManager(color_manager());
  connect(vw, &ViewerWindow::destroyed, this, &ViewerWidget::WindowAboutToClose);
  connect(vw->display_widget(), &ViewerDisplayWidget::customContextMenuRequested, this, &ViewerWidget::ShowContextMenu);

  if (GetConnectedNode()) {
    vw->SetResolution(GetConnectedNode()->video_params().width(), GetConnectedNode()->video_params().height());
  }

  vw->display_widget()->SetImage(display_widget_->last_loaded_buffer());

  windows_.append(vw);
}

void ViewerWidget::ForceUpdate()
{
  // Hack that forces the viewer to update
  UpdateTextureFromNode(GetTime());
}

void ViewerWidget::SetGizmos(Node *node)
{
  display_widget_->SetTimeTarget(GetConnectedNode());
  display_widget_->SetGizmos(node);
}

void ViewerWidget::UpdateTextureFromNode(const rational& time)
{
  if (!FrameExistsAtTime(time)) {
    // There is definitely no frame here, we can immediately flip to showing nothing
    SetDisplayImage(nullptr, false);
    return;
  }

  {
    QMutexLocker locker(playback_queue_.lock());

    while (!playback_queue_.isEmpty()) {
      const ViewerPlaybackFrame& pf = playback_queue_.first();

      if (pf.timestamp == time) {

        // Frame was in queue, no need to decode anything
        SetDisplayImage(pf.frame, true);
        return;

      } else {

        // Skip this frame
        playback_queue_.removeFirst();
        RequestNextFrameForQueue();

      }
    }
  }

  if (IsPlaying()) {
    qDebug() << "Playback queue couldn't keep up - falling back to realtime decoding";
  }

  // Frame was not in queue, will require decoding
  QFutureWatcher<FramePtr>* watcher = new QFutureWatcher<FramePtr>();

  connect(watcher,
          &QFutureWatcher<FramePtr>::finished,
          this,
          &ViewerWidget::RendererGeneratedFrame);

  watcher->setFuture(renderer_->RenderFrame(time, true));
}

void ViewerWidget::PlayInternal(int speed, bool in_to_out_only)
{
  Q_ASSERT(speed != 0);

  if (timebase().isNull()) {
    qWarning() << "ViewerWidget can't play with an invalid timebase";
    return;
  }

  playback_speed_ = speed;
  play_in_to_out_only_ = in_to_out_only;

  QString audio_fn = GetConnectedNode()->audio_playback_cache()->GetCacheFilename();
  if (!audio_fn.isEmpty()) {
    AudioManager::instance()->SetOutputParams(GetConnectedNode()->audio_playback_cache()->GetParameters());
    AudioManager::instance()->StartOutput(audio_fn,
                                          GetConnectedNode()->audio_playback_cache()->GetParameters().time_to_bytes(GetTime()),
                                          playback_speed_);
  }

  int64_t start_time = ruler()->GetTime();

  playback_queue_next_frame_ = start_time;
  FillPlaybackQueue();

  playback_timer_.Start(start_time, playback_speed_, timebase_dbl());

  foreach (ViewerWindow* window, windows_) {
    window->Play(start_time, playback_speed_, timebase());
  }

  controls_->ShowPauseButton();

  playback_is_audio_only_ = (stack_->currentWidget() != sizer_ || !isVisible());

  if (playback_is_audio_only_) {
    connect(AudioManager::instance(), &AudioManager::OutputNotified, this, &ViewerWidget::PlaybackTimerUpdate);
  } else {
    connect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWidget::PlaybackTimerUpdate);
  }
}

void ViewerWidget::PushScrubbedAudio()
{
  if (!IsPlaying() && Config::Current()["AudioScrubbing"].toBool()) {
    // Get audio src device from renderer
    QString audio_fn = GetConnectedNode()->audio_playback_cache()->GetCacheFilename();
    QFile audio_src(audio_fn);

    if (audio_src.open(QFile::ReadOnly)) {
      const AudioRenderingParams& params = GetConnectedNode()->audio_playback_cache()->GetParameters();

      // FIXME: Hardcoded scrubbing interval (20ms)
      int size_of_sample = params.time_to_bytes(rational(20, 1000));

      // Push audio
      audio_src.seek(params.time_to_bytes(GetTime()));
      QByteArray frame_audio = audio_src.read(size_of_sample);
      AudioManager::instance()->SetOutputParams(params);
      AudioManager::instance()->PushToOutput(frame_audio);

      audio_src.close();
    }
  }
}

int ViewerWidget::CalculateDivider()
{
  if (GetConnectedNode() && Config::Current()["AutoSelectDivider"].toBool()) {
    int long_side_of_video = qMax(GetConnectedNode()->video_params().width(), GetConnectedNode()->video_params().height());
    int long_side_of_widget = qMax(display_widget_->width(), display_widget_->height());

    return qMax(1, long_side_of_video / long_side_of_widget);
  }

  return divider_;
}

void ViewerWidget::UpdateMinimumScale()
{
  if (!GetConnectedNode()) {
    return;
  }

  if (GetConnectedNode()->GetLength().isNull()) {
    // Avoids divide by zero
    SetMinimumScale(0);
  } else {
    SetMinimumScale(static_cast<double>(ruler()->width()) / GetConnectedNode()->GetLength().toDouble());
  }
}

void ViewerWidget::SetColorTransform(const ColorTransform &transform, ViewerDisplayWidget *sender)
{
  sender->SetColorTransform(transform);
}

void ViewerWidget::FillPlaybackQueue()
{
  // FIXME: Replace with an asynchronous approach
  while (playback_queue_.size() < 8) {
    rational next_time = Timecode::timestamp_to_time(playback_queue_next_frame_,
                                                     timebase());
    playback_queue_next_frame_ += playback_speed_;
    QFuture<FramePtr> future = renderer_->RenderFrame(next_time, false);
    future.waitForFinished();
    playback_queue_.AppendTimewise({future.result()->timestamp(), future.result()}, playback_speed_);

    foreach (ViewerWindow* window, windows_) {
      window->queue()->AppendTimewise({future.result()->timestamp(), future.result()}, playback_speed_);
    }
  }
}

QString ViewerWidget::GetCachedFilenameFromTime(const rational &time)
{
  if (FrameExistsAtTime(time)) {
    QByteArray hash = GetConnectedNode()->video_frame_cache()->GetHash(time);

    if (!hash.isEmpty()) {
      return GetConnectedNode()->video_frame_cache()->CachePathName(
            hash,
            PixelFormat::instance()->GetConfiguredFormatForMode(RenderMode::kOffline));
    }
  }

  return QString();
}

bool ViewerWidget::FrameExistsAtTime(const rational &time)
{
  return GetConnectedNode() && time < GetConnectedNode()->GetLength();
}

FramePtr ViewerWidget::DecodeCachedImage(const QString &fn)
{
  FramePtr frame = nullptr;

  if (!fn.isEmpty() && QFileInfo::exists(fn)) {
    auto input = OIIO::ImageInput::open(fn.toStdString());

    if (input) {

      PixelFormat::Format image_format = PixelFormat::OIIOFormatToOliveFormat(input->spec().format,
                                                                              input->spec().nchannels == kRGBAChannels);

      frame = Frame::Create();

      frame->set_video_params(VideoRenderingParams(input->spec().width,
                                                   input->spec().height,
                                                   image_format));

      frame->allocate();

      input->read_image(input->spec().format,
                        frame->data(),
                        OIIO::AutoStride,
                        frame->linesize_bytes());

      input->close();

#if OIIO_VERSION < 10903
      OIIO::ImageInput::destroy(input);
#endif

    } else {
      qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
    }
  }

  return frame;
}

void ViewerWidget::SetDisplayImage(FramePtr frame, bool main_only)
{
  display_widget_->SetImage(frame);

  if (!main_only) {
    foreach (ViewerWindow* vw, windows_) {
      vw->display_widget()->SetImage(frame);
    }
  }

  emit LoadedBuffer(frame.get());
}

void ViewerWidget::RequestNextFrameForQueue()
{
  rational next_time = Timecode::timestamp_to_time(playback_queue_next_frame_,
                                                   timebase());

  if (!FrameExistsAtTime(next_time)) {
    return;
  }

  playback_queue_next_frame_ += playback_speed_;

  QFutureWatcher<FramePtr>* watcher = new QFutureWatcher<FramePtr>();
  connect(watcher,
          &QFutureWatcher<FramePtr>::finished,
          this,
          &ViewerWidget::RendererGeneratedFrameForQueue);
  watcher->setFuture(renderer_->RenderFrame(next_time, false));
}

void ViewerWidget::UpdateStack()
{
  if (GetConnectedNode()
      && !GetConnectedNode()->texture_input()->IsConnected()
      && GetConnectedNode()->samples_input()->IsConnected()) {
    // If we have a node AND video is disconnected AND audio is connected, show waveform view
    stack_->setCurrentWidget(waveform_view_);
  } else {
    // Otherwise show regular display
    stack_->setCurrentWidget(sizer_);
  }
}

void ViewerWidget::ContextMenuSetFullScreen(QAction *action)
{
  SetFullScreen(QGuiApplication::screens().at(action->data().toInt()));
}

void ViewerWidget::ContextMenuDisableSafeMargins()
{
  context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(false));
}

void ViewerWidget::ContextMenuSetSafeMargins()
{
  context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(true));
}

void ViewerWidget::ContextMenuSetCustomSafeMargins()
{
  QString s;

  forever {
    bool ok;

    s = QInputDialog::getText(this,
                              tr("Safe Margins"),
                              tr("Enter custom ratio (e.g. \"4:3\", \"16/9\", etc.):"),
                              QLineEdit::Normal,
                              s,
                              &ok);

    if (!ok) {
      // User cancelled dialog, do nothing
      return;
    }

    QStringList ratio_components = s.split(QRegExp(QStringLiteral(":|;|\\/")));

    if (ratio_components.size() == 2) {
      bool numer_ok, denom_ok;

      double num = ratio_components.at(0).toDouble(&numer_ok);
      double den = ratio_components.at(1).toDouble(&denom_ok);

      if (numer_ok
          && denom_ok
          && num > 0) {
        // Exit loop and set this ratio
        context_menu_widget_->SetSafeMargins(ViewerSafeMarginInfo(true, num / den));
        return;
      }
    }

    QMessageBox::warning(this,
                         tr("Invalid custom ratio"),
                         tr("Failed to parse \"%1\" into an aspect ratio. Please format a "
                            "rational fraction with a ':' or a '/' separator.").arg(s),
                         QMessageBox::Ok);
  }
}

void ViewerWidget::WindowAboutToClose()
{
  windows_.removeOne(static_cast<ViewerWindow*>(sender()));
}

void ViewerWidget::ContextMenuScopeTriggered(QAction *action)
{
  emit RequestScopePanel(static_cast<ScopePanel::Type>(action->data().toInt()));
}

void ViewerWidget::RendererGeneratedFrame()
{
  QFutureWatcher<FramePtr>* watcher = static_cast<QFutureWatcher<FramePtr>*>(sender());
  FramePtr frame = watcher->result();
  watcher->deleteLater();

  SetDisplayImage(frame, false);
}

void ViewerWidget::RendererGeneratedFrameForQueue()
{
  QFutureWatcher<FramePtr>* watcher = static_cast<QFutureWatcher<FramePtr>*>(sender());
  FramePtr frame = watcher->result();
  watcher->deleteLater();

  // Ignore this signal if we've paused now
  if (IsPlaying()) {
    playback_queue_.AppendTimewise({frame->timestamp(), frame}, playback_speed_);
  }
}

void ViewerWidget::UpdateRendererParameters()
{
  RenderMode::Mode render_mode = RenderMode::kOffline;

  renderer_->SetDivider(divider_);
  renderer_->SetMode(render_mode);
  renderer_->SetPixelFormat(PixelFormat::instance()->GetConfiguredFormatForMode(render_mode));

  display_widget_->SetVideoParams(GetConnectedNode()->video_params());

  renderer_->SetSampleFormat(SampleFormat::kInternalFormat);
}

void ViewerWidget::ShowContextMenu(const QPoint &pos)
{
  Menu menu(static_cast<QWidget*>(sender()));

  context_menu_widget_ = static_cast<ViewerDisplayWidget*>(sender());

  // Color options
  if (context_menu_widget_->color_manager() && color_menu_enabled_) {
    {
      Menu* ocio_display_menu = context_menu_widget_->GetDisplayMenu(&menu);
      menu.addMenu(ocio_display_menu);
    }

    {
      Menu* ocio_view_menu = context_menu_widget_->GetViewMenu(&menu);
      menu.addMenu(ocio_view_menu);
    }

    {
      Menu* ocio_look_menu = context_menu_widget_->GetLookMenu(&menu);
      menu.addMenu(ocio_look_menu);
    }

    menu.addSeparator();
  }

  {
    // Playback resolution
    Menu* playback_resolution_menu = new Menu(tr("Resolution"), &menu);
    menu.addMenu(playback_resolution_menu);

    playback_resolution_menu->addAction(tr("Full"))->setData(1);
    int dividers[] = {2, 4, 8, 16};
    for (int i=0;i<4;i++) {
      playback_resolution_menu->addAction(tr("1/%1").arg(dividers[i]))->setData(dividers[i]);
    }
    connect(playback_resolution_menu, &QMenu::triggered, this, &ViewerWidget::SetDividerFromMenu);

    foreach (QAction* a, playback_resolution_menu->actions()) {
      a->setCheckable(true);
      if (a->data() == divider_) {
        a->setChecked(true);
      }
    }
  }

  {
    // Viewer Zoom Level
    Menu* zoom_menu = new Menu(tr("Zoom"), &menu);
    menu.addMenu(zoom_menu);

    int zoom_levels[] = {10, 25, 50, 75, 100, 150, 200, 400};
    zoom_menu->addAction(tr("Fit"))->setData(0);
    for (int i=0;i<8;i++) {
      zoom_menu->addAction(tr("%1%").arg(zoom_levels[i]))->setData(zoom_levels[i]);
    }

    connect(zoom_menu, &QMenu::triggered, this, &ViewerWidget::SetZoomFromMenu);
  }

  {
    // Full Screen Menu
    Menu* full_screen_menu = new Menu(tr("Full Screen"), &menu);
    menu.addMenu(full_screen_menu);

    for (int i=0;i<QGuiApplication::screens().size();i++) {
      QScreen* s = QGuiApplication::screens().at(i);

      QAction* a = full_screen_menu->addAction(tr("Screen %1: %2x%3").arg(QString::number(i),
                                                                          QString::number(s->size().width()),
                                                                          QString::number(s->size().height())));

      a->setData(i);
    }

    connect(full_screen_menu, &QMenu::triggered, this, &ViewerWidget::ContextMenuSetFullScreen);
  }

  menu.addSeparator();

  {
    // Scopes
    Menu* scopes_menu = new Menu(tr("Scopes"), &menu);
    menu.addMenu(scopes_menu);

    for (int i=0;i<ScopePanel::kTypeCount;i++) {
      QAction* scope_action = scopes_menu->addAction(ScopePanel::TypeToName(static_cast<ScopePanel::Type>(i)));
      scope_action->setData(i);
    }

    connect(scopes_menu, &Menu::triggered, this, &ViewerWidget::ContextMenuScopeTriggered);
  }

  menu.addSeparator();

  {
    // Safe Margins
    Menu* safe_margin_menu = new Menu(tr("Safe Margins"), &menu);
    menu.addMenu(safe_margin_menu);

    QAction* safe_margin_off = safe_margin_menu->addAction(tr("Off"));
    safe_margin_off->setCheckable(true);
    safe_margin_off->setChecked(!context_menu_widget_->GetSafeMargin().is_enabled());
    connect(safe_margin_off, &QAction::triggered, this, &ViewerWidget::ContextMenuDisableSafeMargins);

    QAction* safe_margin_on = safe_margin_menu->addAction(tr("On"));
    safe_margin_on->setCheckable(true);
    safe_margin_on->setChecked(context_menu_widget_->GetSafeMargin().is_enabled() && !context_menu_widget_->GetSafeMargin().custom_ratio());
    connect(safe_margin_on, &QAction::triggered, this, &ViewerWidget::ContextMenuSetSafeMargins);

    QAction* safe_margin_custom = safe_margin_menu->addAction(tr("Custom Aspect"));
    safe_margin_custom->setCheckable(true);
    safe_margin_custom->setChecked(context_menu_widget_->GetSafeMargin().is_enabled() && context_menu_widget_->GetSafeMargin().custom_ratio());
    connect(safe_margin_custom, &QAction::triggered, this, &ViewerWidget::ContextMenuSetCustomSafeMargins);
  }

  menu.exec(static_cast<QWidget*>(sender())->mapToGlobal(pos));
}

void ViewerWidget::Play(bool in_to_out_only)
{
  if (in_to_out_only
      && GetConnectedTimelinePoints()
      && GetConnectedTimelinePoints()->workarea()->enabled()) {
    // Jump to in point
    SetTimeAndSignal(Timecode::time_to_timestamp(GetConnectedTimelinePoints()->workarea()->in(), timebase()));
  }

  PlayInternal(1, in_to_out_only);
}

void ViewerWidget::Play()
{
  Play(false);
}

void ViewerWidget::Pause()
{
  if (IsPlaying()) {
    AudioManager::instance()->StopOutput();
    playback_speed_ = 0;
    controls_->ShowPlayButton();

    if (playback_is_audio_only_) {
      disconnect(AudioManager::instance(), &AudioManager::OutputNotified, this, &ViewerWidget::PlaybackTimerUpdate);
    } else {
      disconnect(display_widget_, &ViewerDisplayWidget::frameSwapped, this, &ViewerWidget::PlaybackTimerUpdate);
    }

    foreach (ViewerWindow* window, windows_) {
      window->Pause();
    }

    {
      playback_queue_.lock()->lock();
      playback_queue_.clear();
      playback_queue_.lock()->unlock();
    }
  }
}

void ViewerWidget::ShuttleLeft()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    Pause();
  }

  current_speed--;

  if (current_speed == 0) {
    current_speed--;
  }

  PlayInternal(current_speed, false);
}

void ViewerWidget::ShuttleStop()
{
  Pause();
}

void ViewerWidget::ShuttleRight()
{
  int current_speed = playback_speed_;

  if (current_speed != 0) {
    Pause();
  }

  current_speed++;

  if (current_speed == 0) {
    current_speed++;
  }

  PlayInternal(current_speed, false);
}

void ViewerWidget::SetColorTransform(const ColorTransform &transform)
{
  SetColorTransform(transform, display_widget_);
}

void ViewerWidget::SetSignalCursorColorEnabled(bool e)
{
  display_widget_->SetSignalCursorColorEnabled(e);

  foreach (ViewerWindow* vw, windows_) {
    vw->display_widget()->SetSignalCursorColorEnabled(e);
  }
}

void ViewerWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimeBasedWidget::TimebaseChangedEvent(timebase);

  controls_->SetTimebase(timebase);

  controls_->SetTime(ruler()->GetTime());
  LengthChangedSlot(GetConnectedNode() ? GetConnectedNode()->GetLength() : 0);
}

void ViewerWidget::PlaybackTimerUpdate()
{
  int64_t current_time = playback_timer_.GetTimestampNow();

  int64_t min_time, max_time;

  {
    if ((play_in_to_out_only_ || Config::Current()["Loop"].toBool())
        && GetConnectedTimelinePoints()
        && GetConnectedTimelinePoints()->workarea()->enabled()) {

      // If "play in to out" is enabled or we're looping AND we have a workarea, only play the workarea
      min_time = Timecode::time_to_timestamp(GetConnectedTimelinePoints()->workarea()->in(), timebase());
      max_time = Timecode::time_to_timestamp(GetConnectedTimelinePoints()->workarea()->out(), timebase());

    } else {

      // Otherwise set the bounds to the range of the sequence
      min_time = 0;
      max_time = Timecode::time_to_timestamp(GetConnectedNode()->GetLength(), timebase());

    }
  }

  if ((playback_speed_ < 0 && current_time <= min_time)
      || (playback_speed_ > 0 && current_time >= max_time)) {

    // Determine which timestamp we tripped
    int64_t tripped_time;

    if (current_time <= min_time) {
      tripped_time = min_time;
    } else {
      tripped_time = max_time;
    }

    if (Config::Current()["Loop"].toBool()) {

      // If we're looping, jump to the other side of the workarea and continue
      int64_t opposing_time = (tripped_time == min_time) ? max_time : min_time;

      // Cache the current speed
      int current_speed = playback_speed_;

      // Jump to the other side and keep playing at the same speed
      SetTimeAndSignal(opposing_time);
      PlayInternal(current_speed, play_in_to_out_only_);

    } else {

      // Pause at the boundary
      SetTimeAndSignal(tripped_time);

    }

  } else {

    // Sets time, wrapping in this bool ensures we don't pause from setting the time
    time_changed_from_timer_ = true;
    SetTimeAndSignal(current_time);
    time_changed_from_timer_ = false;

  }
}

void ViewerWidget::SizeChangedSlot(int width, int height)
{
  sizer_->SetChildSize(width, height);

  foreach (ViewerWindow* vw, windows_) {
    vw->SetResolution(width, height);
  }
}

void ViewerWidget::LengthChangedSlot(const rational &length)
{
  controls_->SetEndTime(Timecode::time_to_timestamp(length, timebase()));
  UpdateMinimumScale();
}

void ViewerWidget::SetDividerFromMenu(QAction *action)
{
  int divider = action->data().toInt();

  if (divider <= 0) {
    qWarning() << "Tried to set invalid divider:" << divider;
    return;
  }

  divider_ = divider;

  UpdateRendererParameters();
}

void ViewerWidget::SetZoomFromMenu(QAction *action)
{
  sizer_->SetZoom(action->data().toInt());
}

void ViewerWidget::ViewerInvalidatedRange(const TimeRange &range)
{
  if (GetTime() >= range.in() && GetTime() < range.out()) {
    ForceUpdate();
  }
}

OLIVE_NAMESPACE_EXIT
