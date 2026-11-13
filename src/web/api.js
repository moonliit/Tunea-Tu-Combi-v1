// the session secret only exists in the HttpOnly cookie
Module['api'] = (() => {
    let identity = null;
    let status = 'local';
    let epoch = 0;
    let busy = false;
    let notice = '';
    let updateUi = () => {};
    let transientLocalProfileId = '';
    let leaderboardGeneration = 0;
    let leaderboardState = {
        status: 'idle',
        entries: [],
        personalBest: null,
        submission: null,
        championRunRef: '',
        message: ''
    };
    const personalRecordsKey = 'tunea-tu-combi:personal-records-v1';
    const localProfileKey = 'tunea-tu-combi:local-profile-v1';
    const channel = typeof BroadcastChannel !== 'undefined' ? new BroadcastChannel('combi-session') : null;
    const messages = {
        NAME_TAKEN: 'Ese nombre ya está reservado. Elige otro.',
        INVALID_NAME: 'Usa de 3 a 24 letras latinas, números, espacios o guiones.',
        RETRY_RENAME: 'Hubo otro cambio simultáneo. Vuelve a intentarlo.',
        INVALID_SESSION: 'La sesión dejó de ser válida. Reconecta para crear otra identidad.',
        NO_SESSION: 'No hay sesión. Reconecta para crear una identidad.'
    };
    function accept(next) {
        if (identity?.id !== next?.id) epoch++;
        identity = next;
        status = next ? 'ready' : 'local';
    }
    async function request(path, method = 'GET', body) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 8000);
        try {
            const response = await fetch('/api' + path, {
                method, credentials: 'same-origin', cache: 'no-store', signal: controller.signal,
                ...(method !== 'GET' ? { headers: { 'content-type': 'application/json', 'x-combi-request': '1' }, body: JSON.stringify(body ?? {}) } : {})
            });
            const data = await response.json();
            if (!response.ok) throw Object.assign(new Error(data.error), { status: response.status });
            return data;
        } finally { clearTimeout(timer); }
    }
    function fail(error) {
        accept(null);
        notice = messages[error.message] ?? 'Sin conexión confirmada. Puedes jugar localmente; no puedes reservar ni publicar.';
    }
    const lock = fn => typeof navigator !== 'undefined' && navigator.locks
        ? navigator.locks.request('combi-session', fn) : fn();
    async function connect(create = true) {
        if (busy) return;
        busy = true;
        updateUi();
        try {
            await lock(async () => {
                let result;
                try { result = await request('/session'); }
                catch (error) {
                    if (error.status !== 401 || !create) throw error;
                    accept(null);
                    result = await request('/session', 'POST');
                    const confirmed = await request('/session');
                    if (confirmed.identity.id !== result.identity.id) throw new Error('SESSION_CHANGED');
                    result = confirmed;
                    channel?.postMessage('changed');
                }
                accept(result.identity);
                notice = '';
            });
        } catch (error) { fail(error); }
        finally { busy = false; updateUi(); }
    }
    async function rename(name) {
        if (busy || status !== 'ready') return false;
        busy = true;
        updateUi();
        try {
            const expectedId = identity.id;
            const current = await request('/session');
            if (current.identity.id !== expectedId) throw new Error('SESSION_CHANGED');
            const result = await request('/identity/name', 'PUT', { name, expectedId });
            if (result.identity.id !== expectedId) throw new Error('SESSION_CHANGED');
            accept(result.identity);
            leaderboardState = {
                ...leaderboardState,
                entries: leaderboardState.entries.map((entry) =>
                    entry.combiId === result.identity.id
                        ? { ...entry, name: result.identity.name }
                        : entry
                )
            };
            notice = 'Nombre reservado.';
            channel?.postMessage('changed');
            return true;
        } catch (error) {
            if ([400, 409].includes(error.status) && error.message !== 'IDENTITY_CHANGED') notice = messages[error.message] ?? 'No se guardó el nombre.';
            else fail(error);
            return false;
        } finally { busy = false; updateUi(); }
    }
    function snapshot() {
        return { status, id: identity?.id ?? '', name: identity?.name ?? '', epoch, canPublish: false };
    }
    function captureResultScope() {
        return Object.freeze({ id: identity?.id ?? '', epoch });
    }
    async function canSubmitResult(scope) {
        if (!scope?.id || scope.id !== identity?.id || scope.epoch !== epoch) return false;
        await connect(false);
        return status === 'ready' && scope.id === identity?.id && scope.epoch === epoch && !busy;
    }
    function localProfileId() {
        try {
            let id = localStorage.getItem(localProfileKey);
            if (!id) {
                id = crypto.randomUUID();
                localStorage.setItem(localProfileKey, id);
            }
            return id;
        } catch {
            if (!transientLocalProfileId) transientLocalProfileId = crypto.randomUUID();
            return transientLocalProfileId;
        }
    }
    function readPersonalRecords() {
        try {
            const value = JSON.parse(localStorage.getItem(personalRecordsKey) ?? '{}');
            return value && typeof value === 'object' && !Array.isArray(value) ? value : {};
        } catch {
            return {};
        }
    }
    function compareRecords(first, second) {
        return second.score.total - first.score.total ||
            first.statistics.driverTimeMs - second.statistics.driverTimeMs ||
            first.statistics.initialParts - second.statistics.initialParts ||
            first.recordedAt - second.recordedAt;
    }
    function updatePersonalBest(payload) {
        const owner = payload.scope.id || `local:${localProfileId()}`;
        const key = `${owner}|${payload.levelId || 'local'}`;
        const records = readPersonalRecords();
        const candidate = {
            attemptId: payload.attemptId,
            recordedAt: Date.now(),
            statistics: payload.statistics,
            score: payload.localScore
        };
        const previous = records[key];
        const validPrevious = previous?.score && previous?.statistics &&
            Number.isSafeInteger(previous.score.total) &&
            Number.isSafeInteger(previous.statistics.driverTimeMs) &&
            Number.isSafeInteger(previous.statistics.initialParts);
        if (!validPrevious || compareRecords(candidate, previous) < 0) {
            records[key] = candidate;
            try { localStorage.setItem(personalRecordsKey, JSON.stringify(records)); }
            catch {}
        }
        const best = records[key] ?? candidate;
        return { ...best, currentAttempt: best.attemptId === payload.attemptId };
    }
    async function submitRun(payload) {
        const generation = ++leaderboardGeneration;
        const personalBest = updatePersonalBest(payload);
        leaderboardState = {
            status: 'loading',
            entries: [],
            personalBest,
            submission: null,
            championRunRef: '',
            message: ''
        };
        if (!payload.levelId) {
            leaderboardState = {
                ...leaderboardState,
                status: 'unavailable',
                message: 'Este nivel local todavía no tiene ranking global.'
            };
            return;
        }
        try {
            const ownsRun = payload.scope.id &&
                payload.scope.id === identity?.id &&
                payload.scope.epoch === epoch &&
                status === 'ready';
            const result = ownsRun
                ? await request('/leaderboard/attempts', 'POST', {
                    attemptId: payload.attemptId,
                    formatVersion: payload.formatVersion,
                    levelId: payload.levelId,
                    physicsVersion: payload.physicsVersion,
                    rulesVersion: payload.rulesVersion,
                    statistics: payload.statistics
                })
                : await request(`/leaderboard?levelId=${encodeURIComponent(payload.levelId)}`);
            if (generation !== leaderboardGeneration) return;
            leaderboardState = {
                status: 'ready',
                entries: result.entries,
                personalBest,
                submission: result.submission ?? null,
                championRunRef: result.championRunRef ?? '',
                message: ownsRun
                    ? 'Puntaje recalculado por el servidor; la física no fue verificada.'
                    : 'Resultado local no enviado; la identidad de la partida ya no está disponible.'
            };
        } catch {
            if (generation !== leaderboardGeneration) return;
            leaderboardState = {
                ...leaderboardState,
                status: 'unavailable',
                message: 'Ranking global no disponible. El récord personal quedó guardado localmente.'
            };
        }
    }
    function leaderboardSnapshot() {
        return leaderboardState;
    }
    function mount() {
        const panel = document.createElement('section');
        panel.setAttribute('aria-label', 'Identidad de tu combi');
        panel.style.cssText = 'max-width:800px;margin:12px auto;padding:12px;box-sizing:border-box;font:15px sans-serif;background:#eef4e7;color:#203021;text-align:left';
        const state = document.createElement('p');
        state.setAttribute('role', 'status');
        const form = document.createElement('form');
        const label = document.createElement('label');
        label.textContent = 'Nombre de tu combi: ';
        const input = document.createElement('input');
        input.type = 'text'; input.maxLength = 64; input.autocomplete = 'off';
        input.setAttribute('aria-describedby', 'combi-name-rules');
        label.append(input);
        const save = document.createElement('button'); save.type = 'submit'; save.textContent = 'Reservar / cambiar';
        const retry = document.createElement('button'); retry.type = 'button'; retry.textContent = 'Reconectar';
        const rules = document.createElement('p'); rules.id = 'combi-name-rules';
        rules.textContent = '3–24 caracteres: letras latinas (con tildes), números, espacios o guiones; empieza y termina con letra o número. Mayúsculas y espacios sobrantes no distinguen nombres. Unicode NFKC; las tildes sí distinguen.';
        const warning = document.createElement('p');
        warning.textContent = 'Si borras la cookie o vence (1 año), crearás otra identidad y perderás acceso a la anterior desde este navegador. Sus niveles, grabaciones y nombre reservado permanecen. No hay recuperación.';
        const feedback = document.createElement('p'); feedback.setAttribute('aria-live', 'polite');
        form.append(label, save, retry); panel.append(state, form, rules, warning, feedback);
        const canvas = document.querySelector('canvas');
        const container = canvas?.parentElement;
        if (container?.parentElement) container.before(panel); else document.body.prepend(panel);
        for (const event of ['keydown', 'keyup', 'keypress']) panel.addEventListener(event, e => e.stopPropagation());
        let renderedName = null;
        updateUi = () => {
            state.textContent = status === 'ready'
                ? (identity.name ? 'Combi: ' + identity.name : 'Identidad conectada. Todavía no tienes un nombre reservado.')
                : 'Modo local: puedes jugar. Sin nombre reservado ni permisos de publicación.';
            if (identity?.name !== renderedName) { input.value = identity?.name ?? ''; renderedName = identity?.name; }
            save.disabled = busy || status !== 'ready'; retry.disabled = busy;
            feedback.textContent = busy ? 'Conectando…' : notice;
        };
        form.addEventListener('submit', async e => { e.preventDefault(); await rename(input.value); });
        retry.addEventListener('click', () => connect());
        updateUi();
        connect();
        window.addEventListener('focus', () => connect());
        window.addEventListener('pageshow', () => connect());
        setInterval(() => { if (!document.hidden) connect(); }, 30000);
        if (channel) channel.onmessage = () => connect(false);
    }
    if (typeof document !== 'undefined') {
        if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', mount, { once: true });
        else mount();
    }
    return {
        checkHealth: () => request('/health'),
        connect,
        rename,
        snapshot,
        captureResultScope,
        canSubmitResult,
        submitRun,
        leaderboardSnapshot
    };
})();
