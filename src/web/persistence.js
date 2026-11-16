// Keeps /user_data (the drafts of the level editor) in the browser's IndexedDB so
// they survive reloading the page.
Module['persistence'] = (() => {
    let ready = false;
    let running = false;
    let again = false;

    // The game starts only after the saved files are back in memory.
    Module['preRun'] = [].concat(Module['preRun'] || [], () => {
        addRunDependency('user_data');
        const done = (error) => {
            if (error) console.warn('No se pudieron cargar los niveles guardados:', error);
            else ready = true;
            removeRunDependency('user_data');
        };
        try {
            try { FS.mkdir('/user_data'); } catch { /* already there */ }
            FS.mount(IDBFS, {}, '/user_data');
            FS.syncfs(true, done);
        } catch (error) {
            done(error);
        }
    });

    // One write at a time: a save that arrives meanwhile becomes one more write.
    function save() {
        if (!ready) return;
        if (running) { again = true; return; }
        running = true;
        FS.syncfs(false, (error) => {
            running = false;
            if (error) console.warn('No se pudieron guardar los niveles:', error);
            if (again) { again = false; save(); }
        });
    }

    return { save };
})();
