# SONIC GRAIN – note per lo sviluppo

- JUCE 8 audio effect plugin (VST3/AU/AAX). Everything is in `Source/`; the plugin target is defined in the root `CMakeLists.txt`.
- Build + tests: `cmake -B build -DCMAKE_BUILD_TYPE=Release -DSONICGRAIN_BUILD_TESTS=ON -DSONICGRAIN_COPY_AFTER_BUILD=OFF && cmake --build build -j3`, then run `build/Tests/SonicGrainTests_artefacts/Release/SonicGrainTests`. `--snap file.png [tab] [scale]` renders the editor (tab 0-3 = LFO 1-4, 4 = METRO SEQ; scale = pixel density, default 2); look at it after any UI change. The UI is laid out on a fixed 1280x800 grid (`sonic::grid` in `SonicEditor.h`: 16 px margins and panel padding, 12 px gaps) and the editor scales it.
- JUCE: `-DJUCE_DIR=<path>` for a local copy; otherwise FetchContent downloads tag 8.0.10.
- Headless Linux without the X dev packages: export `CXXFLAGS="-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0 -DJUCE_USE_XRENDER=0 -DJUCE_ALSA=0 -DJUCE_JACK=0"` before configuring. Use `-j3`: higher values run out of memory.
- Never change these, or hosts lose saved sessions: plugin code `Sgrn`, manufacturer code `Gsof`, parameter IDs, or the order of `kDestIds` / `kSrcNames` in `SonicProcessor.h`. Add new parameters at the end.
- On/off parameters are 2-item choices (`bparam` in `IDMLookAndFeel.h`), so pluginval strictness 8 passes the state-restore test.
- Release: update `CHANGELOG.md` and `project(... VERSION x.y.z)` in `CMakeLists.txt`, then push tag `vx.y.z`. CI builds and publishes the Release.
- The user is Italian: user-facing docs (README, CHANGELOG, scripts) are in Italian.
