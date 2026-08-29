# v0.4 Color, Audio & Performance — Community Testing Tasks

This document breaks the v0.4 manual test plan into small, volunteer-friendly tasks. Pick one, run it, and check the box when it passes. If it fails, file an issue using the bug template at the bottom.

## How to contribute

1. Pick an unchecked task below.
2. Build or download the latest v0.4.0-alpha build for your platform.
3. Follow the steps and compare against the pass criteria.
4. Check the box in your local copy and report the result in a comment/issue.

---

## 0. Pre-flight checks

- [ ] **0.1 Import sample media**
  - Create a new project.
  - Import a 4K H.264/HEVC clip with audio, plus a WAV file.
  - Save the project, close Oak, reopen it.
  - **Pass:** all clips are still online and no crash on reopen.

- [ ] **0.2 Smoke test on clean build**
  - Launch Oak from a fresh build or CI package.
  - Open the About dialog and confirm the version reads `v0.4.0-alpha`.
  - **Pass:** app launches without crash and version is correct.

---

## 1. LUT & Color

- [ ] **1.1 Apply a `.cube` LUT**
  - Add a clip to the timeline.
  - Add an OCIO LUT effect and load a `.cube` LUT.
  - Play 5 seconds and scrub the timeline.
  - **Pass:** colors change visibly; no hang; scrubbing updates the viewer.

- [ ] **1.2 Apply a `.3dl` LUT**
  - Repeat 1.1 with a `.3dl` LUT.
  - Toggle the effect on/off.
  - **Pass:** `.3dl` works; toggle is instant; no crash.

- [ ] **1.3 Corrupt LUT handling**
  - Try to load a malformed LUT file.
  - Observe the error UI, then continue playing the original clip.
  - Save, close, and reopen the project.
  - **Pass:** no crash; original clip still plays; project reopens cleanly.

- [ ] **1.4 Three-way color wheels**
  - Open the three-way color wheels panel.
  - Adjust Shadows, Midtones, and Highlights on a color-chart clip.
  - Play, pause, and step frames.
  - **Pass:** each region changes the image; undo/redo works; values persist after reopen.

- [ ] **1.5 LUT + proxy combination**
  - Generate a proxy for a heavy clip and enable `Use Proxy`.
  - Apply a LUT and three-way adjustments.
  - Disable proxy and compare the same frame.
  - **Pass:** color looks consistent; export uses full-resolution source, not proxy.

---

## 2. Scopes

- [ ] **2.1 Waveform scope**
  - Open the Scope panel and select Waveform.
  - Play `color_chart.mov` and adjust brightness/exposure.
  - **Pass:** waveform updates with current frame and grade; no lag buildup.

- [ ] **2.2 Vectorscope**
  - Switch the Scope panel to Vectorscope.
  - Use a frame with skin tones and saturated colors.
  - Adjust saturation or color wheels.
  - **Pass:** plot moves with saturation/hue changes; switching panels does not crash.

- [ ] **2.3 Histogram**
  - Switch the Scope panel to Histogram.
  - Adjust brightness/contrast on a high-contrast or grayscale frame.
  - Play and pause on different frames.
  - **Pass:** histogram updates correctly; no ghosting from previous frames.

- [ ] **2.4 Scope switching stress**
  - Switch between Waveform, Vectorscope, and Histogram 20 times.
  - While switching, scrub the timeline and adjust a grade.
  - **Pass:** no OpenGL/shader crash; UI stays responsive.

---

## 3. Waveform Auto-Sync

- [ ] **3.1 Dual-system audio sync**
  - Add a video clip with in-camera audio and a separate external WAV.
  - Offset the external audio by 1–3 seconds.
  - Select both clips and run `Sync by Waveform`.
  - Play the slate/lip-sync section.
  - **Pass:** slate peaks align; lip-sync error is ≤ 1 frame; no clips deleted.

- [ ] **3.2 Multi-clip sync**
  - Add 3 video clips and 3 external audio tracks with different offsets.
  - Select all and run waveform sync.
  - **Pass:** each pair aligns; unmatched clips stay in place or report failure.

- [ ] **3.3 Low-quality reference audio**
  - Use a noisy or quiet reference track for waveform sync.
  - **Pass:** if sync fails, it does not create a false alignment; user gets a warning or the clip stays put.

- [ ] **3.4 Sync persistence**
  - After a successful sync, save the project, close Oak, and reopen.
  - Play the sync point.
  - **Pass:** clip positions are preserved; sync does not drift on reopen.

---

## 4. BWF Timecode Sync

- [ ] **4.1 Read BWF timecode**
  - Import a WAV file with BWF timecode metadata.
  - Check the clip properties/timecode field.
  - Place it on the timeline and run `Sync by Timecode`.
  - **Pass:** start timecode is recognized; sync does not jump to an extreme timestamp.

- [ ] **4.2 BWF + video timecode alignment**
  - Import a video clip and a BWF audio file with matching timecode.
  - Run `Sync by Source Timecode`.
  - **Pass:** audio and video align; timeline position matches the timecode difference.

- [ ] **4.3 Missing timecode fallback**
  - Select a plain WAV without BWF timecode and run `Sync by Timecode`.
  - **Pass:** a clear error or skip; no extreme offset applied.

---

## 5. Audio Meters

- [ ] **5.1 VU meter response**
  - Add `noisy_dialogue.wav` to the timeline.
  - Open the audio meter panel and play silence, dialogue, and music sections.
  - **Pass:** needle moves with volume; drops on silence; settles after stop.

- [ ] **5.2 LUFS reading**
  - Play 30 seconds of dialogue or music.
  - Watch the short-term/integrated LUFS readout.
  - Change clip gain and replay.
  - **Pass:** LUFS follows gain changes; no NaN, inf, or wild jumps.

- [ ] **5.3 Multi-channel audio**
  - Import stereo and multi-channel clips.
  - Play and mute/lower one clip.
  - **Pass:** channels display correctly; mute/gain changes are instant.

---

## 6. Proxy Media

- [ ] **6.1 Generate proxy**
  - Add an 8K or heavy 4K clip to the timeline.
  - Right-click → `Proxy > Generate Proxy`.
  - Watch the task list and proxy cache folder.
  - **Pass:** `.working` file becomes `.mp4`; `.working` is cleaned; UI does not block.

- [ ] **6.2 Enable/disable proxy**
  - After generation, check `Proxy > Use Proxy` and play.
  - Uncheck `Use Proxy` and play again.
  - **Pass:** proxy playback works; disabling returns to original; missing proxy falls back safely.

- [ ] **6.3 Reveal and delete proxy**
  - Run `Proxy > Reveal Proxy` and confirm the folder opens.
  - Run `Proxy > Delete Proxy` and play again.
  - **Pass:** reveal points to cache/proxy; delete removes proxy and working files; playback falls back.

- [ ] **6.4 Proxy state persistence**
  - Generate, enable proxy, and save.
  - Close and reopen the project.
  - **Pass:** `Use Proxy` state is restored; proxy file is used if it exists.

- [ ] **6.5 Proxy failure path**
  - Temporarily break `ffmpeg` or use an unreadable source.
  - Try to generate a proxy.
  - **Pass:** clear failure message; `.working` cleaned; original clip still plays.

- [ ] **6.6 Export ignores proxy**
  - Generate a 720p proxy for a 4K clip and enable it.
  - Export a 4K segment.
  - **Pass:** exported file is 4K and uses full-resolution source.

---

## 7. Hardware-Accelerated Export

- [ ] **7.1 NVENC export (NVIDIA only)**
  - On an NVIDIA machine, choose H.264/H.265 NVENC in export settings.
  - Export a 30-second 4K segment.
  - Check codec with `ffprobe`.
  - **Pass:** export succeeds; codec is correct; unavailable hardware reports a clear error.

- [ ] **7.2 VideoToolbox export (macOS only)**
  - On macOS, choose VideoToolbox H.264/H.265.
  - Export a 30-second 4K segment.
  - **Pass:** export succeeds; output plays; system load looks like hardware encoding.

- [ ] **7.3 Hardware export fallback**
  - Select a hardware encoder your machine does not support.
  - Attempt export.
  - **Pass:** understandable failure; option to switch to software encoding.

---

## 8. Batch Render Queue

- [ ] **8.1 Multi-job queue**
  - Create 3 sequences: a short clip, one with a LUT, one with proxy.
  - Add all 3 to the batch render queue and start.
  - **Pass:** jobs run in order; each output file is created; one failure does not crash the app.

- [ ] **8.2 Queue cancel**
  - Add a long export job, start it, then cancel immediately.
  - Add a short job and run it.
  - **Pass:** cancel leaves no dead state; next job runs; partial file status is clear.

- [ ] **8.3 Queue project save**
  - Configure several queue jobs and save the project.
  - Close and reopen Oak.
  - **Pass:** behavior is consistent: queue is either restored cleanly or empty, never half-broken.

---

## 9. Regression & Path Tests

- [ ] **9.1 Full editing chain**
  - Make a 60-second sequence mixing 4K/8K, external audio, LUT, three-way grade, and proxy.
  - Waveform-sync some clips.
  - Open Scopes and audio meters and play the whole sequence.
  - Export a software-encoded version.
  - **Pass:** no crash; exported file is in sync; color and audio match preview.

- [ ] **9.2 Chinese and space characters in paths**
  - Put project, media, cache, and export target in a path like `Oak v04 测试/素材 A`.
  - Repeat proxy generation, LUT load, and export.
  - **Pass:** paths work; `Reveal Proxy` and exported file open correctly.

- [ ] **9.3 Long playback stability**
  - Open a 4K/8K project and loop playback for 20 minutes.
  - Toggle proxy, scopes, and audio meters during playback.
  - **Pass:** memory does not grow uncontrollably; app remains responsive; save works after stop.

---

## 10. Graphics Backend (OpenGL/Vulkan)

> Only test Vulkan if `vulkaninfo --summary` reports a working instance and device. Otherwise, mark Vulkan tasks as skipped and test OpenGL fallback only.

- [ ] **10.1 Default OpenGL backend**
  - Delete/backup user config and launch Oak.
  - Open Preferences > Behavior > Rendering.
  - **Pass:** default backend is OpenGL; viewer, scopes, grade, and playback work.

- [ ] **10.2 Vulkan runtime preflight**
  - Run `vulkaninfo --summary` and note GPU, driver, and API version.
  - Confirm `liboakvulkan.so/dylib/dll` exists next to the Oak binary.
  - Run `olive-gtest --gtest_filter='DynamicRenderBackend.*'`.
  - **Pass:** on a valid Vulkan setup, backend load/upload/download/Blit tests run; on invalid setups, tests SKIP and OpenGL fallback passes.

- [ ] **10.3 Switch to Vulkan and restart**
  - Select `Vulkan (experimental)` in Preferences, save, and restart Oak.
  - Reopen Preferences and confirm Vulkan is still selected.
  - Import and play a 4K clip.
  - **Pass:** no crash; logs show actual backend matches selection; fallback to OpenGL is explicit if Vulkan fails.

- [ ] **10.4 Vulkan viewer playback**
  - On a working Vulkan setup, import two clips and play 10 seconds.
  - Pause, step frames, scrub, and resize the viewer.
  - **Pass:** viewer displays correctly; no black screen, flicker, or hang.

- [ ] **10.5 Vulkan LUT/grade consistency**
  - In Vulkan mode, load a LUT and make a strong three-way adjustment.
  - Note the viewer appearance.
  - Switch to OpenGL, reopen the same project at the same frame.
  - **Pass:** colors are directionally consistent; no channel swap or gamma reversal.

- [ ] **10.6 Vulkan proxy export**
  - In Vulkan mode, generate and enable a proxy for a heavy clip.
  - Export a software-encoded segment.
  - **Pass:** export uses full-resolution source; output is playable and in sync.

- [ ] **10.7 Vulkan scope behavior**
  - In Vulkan mode, open Waveform, Vectorscope, and Histogram while grading.
  - **Pass:** no crash; if scopes are backend-neutral and skip, that is documented; OpenGL scopes still work.

- [ ] **10.8 Missing Vulkan runtime fallback**
  - On a machine without Vulkan, select Vulkan and restart.
  - Open a project and play.
  - **Pass:** app starts; logs say Vulkan unavailable and OpenGL fallback active; Preferences can switch back to OpenGL.

- [ ] **10.9 Switch back from Vulkan to OpenGL**
  - Change Preferences from Vulkan to OpenGL, save, and restart.
  - Play and export the same project.
  - **Pass:** backend shows OpenGL; playback and export work; no stale Vulkan state.

- [ ] **10.10 Dynamic OpenGL backend load**
  - On an experimental dynamic-backend build, confirm `liboakgl` exists.
  - Select OpenGL and restart.
  - Play, scrub, open scopes, and exit.
  - **Pass:** logs show dynamic OpenGL loaded; behavior matches default OpenGL; clean exit.

- [ ] **10.11 Missing dynamic backend handling**
  - Temporarily rename `liboakgl` / `oakgl.dll`.
  - Launch Oak and open a project.
  - **Pass:** no silent crash; logs report backend load failure; user can restore the library or use a non-dynamic build.

---

## Bug report template

When a task fails, file an issue with:

- Platform, GPU, driver, FFmpeg version.
- Oak commit hash.
- Task number (e.g., **1.1**).
- Project file path and media types used.
- Exact reproduction steps.
- Expected vs. actual result.
- Is it reproducible every time?
- For export failures: attach `ffprobe` output and export settings screenshot.
- For Vulkan failures: attach `vulkaninfo --summary`, backend log, and whether OpenGL fallback occurred.

## Release blockers

Before calling v0.4.0-alpha ready, the following must pass:

- Pre-flight, LUT, three-way wheels, all three scopes, waveform sync, BWF sync, audio meters, proxy generate/enable/delete, and software export.
- At least one hardware encoder environment (NVENC or VideoToolbox).
- OpenGL/Vulkan backend selection and persistence; Vulkan real rendering on at least one valid setup; clean OpenGL fallback when Vulkan is unavailable.
- Batch queue multi-job and cancel tests.
- Full editing chain regression test.
- No silent crashes remain; every failure is filed as an issue or documented as a known limitation.
