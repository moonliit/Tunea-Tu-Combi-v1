// the session secret only exists in the HttpOnly cookie
Module['api'] = (() => {
    const matchaStyles = document.createElement('style');
    matchaStyles.textContent = `@font-face{font-family:"Matcha Style";src:url("./MatchaStyle.ttf") format("truetype")}
        .combi-matcha,.combi-matcha button,.combi-matcha input{font-family:"Matcha Style",sans-serif}`;
    document.head.append(matchaStyles);
    let identity = null;
    let status = 'local';
    let epoch = 0;
    let unlockedLevel = 1;
    let busy = false;
    let notice = '';
    let updateUi = () => {};
    let transientLocalProfileId = '';
    let leaderboardGeneration = 0;
    let publicationRevision = 0;
    let publicationState = {
        status: 'idle',
        message: '',
        revision: 0,
        play: null,
        copy: null
    };
    const catalogState = {
        items: [],
        mine: false,
        query: '',
        currentCursor: null,
        nextCursor: null,
        previous: [],
        loading: false,
        error: ''
    };
    let catalogSearchTimer = null;
    let catalogGeneration = 0;
    let leaderboardState = {
        status: 'idle',
        entries: [],
        personalBest: null,
        submission: null,
        championRunRef: '',
        championRecording: { status: 'unavailable' },
        message: ''
    };
    let pendingChampionCapture = null;
    let recordingUploadBusy = false;
    let recordingRetryCount = 0;
    let recordingRetryTimer = null;
    let ghostState = { status: 'idle', message: '', capture: null };
    let ghostGeneration = 0;
    const ghostCache = new Map();
    // One parsed trace avoids duplicate transfers when re-entering a level
    // without retaining several potentially large (up to 32 MiB) JSON trees.
    const ghostCacheLimit = 1;
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
        const timer = setTimeout(
            () => controller.abort(),
            method === 'POST' && path === '/levels' ? 20000 : 8000
        );
        try {
            const response = await fetch('/api/v1' + path, {
                method, credentials: 'same-origin', cache: 'no-store', signal: controller.signal,
                ...(method !== 'GET' ? {
                    headers: {
                        'x-combi-request': '1',
                        ...(body !== undefined ? { 'content-type': 'application/json' } : {})
                    },
                    ...(body !== undefined ? { body: JSON.stringify(body) } : {})
                } : {})
            });
            const data = await response.json();
            if (!response.ok) throw Object.assign(new Error(data.error), { status: response.status });
            return data;
        } finally { clearTimeout(timer); }
    }
    const levelPath = (levelId) => `/levels/${encodeURIComponent(levelId)}`;
    const publicLevelPath = (id) => levelPath(`public:${id}`);
    function fail(error) {
        accept(null);
        notice = messages[error.message] ?? '';
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
                if (Number.isInteger(result.progress?.unlockedLevel))
                    unlockedLevel = result.progress.unlockedLevel;
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
            const result = await request(`/identities/${encodeURIComponent(expectedId)}/name`, 'PUT', { name });
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
            loadCatalog(catalogState.currentCursor);
            channel?.postMessage('changed');
            return true;
        } catch (error) {
            if ([400, 409].includes(error.status) && error.message !== 'IDENTITY_CHANGED') notice = messages[error.message] ?? 'No se guardó el nombre.';
            else fail(error);
            return false;
        } finally { busy = false; updateUi(); }
    }
    function snapshot() {
        return { status, id: identity?.id ?? '', name: identity?.name ?? '', epoch, unlockedLevel, canPublish: false };
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
            championRecording: { status: 'unavailable' },
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
                ? await request(`${levelPath(payload.levelId)}/attempts/${encodeURIComponent(payload.attemptId)}`, 'PUT', {
                    formatVersion: payload.formatVersion,
                    physicsVersion: payload.physicsVersion,
                    rulesVersion: payload.rulesVersion,
                    statistics: payload.statistics,
                    score: payload.localScore
                })
                : await request(`${levelPath(payload.levelId)}/leaderboard`);
            if (generation !== leaderboardGeneration) return;
            leaderboardState = {
                status: 'ready',
                entries: result.entries,
                personalBest,
                submission: result.submission ?? null,
                championRunRef: result.championRunRef ?? '',
                championRecording: result.championRecording ?? { status: 'unavailable' },
                message: ownsRun
                    ? 'Puntaje enviado al ranking. La física no fue verificada.'
                    : 'Resultado local no enviado. La identidad de la partida ya no está disponible.'
            };
            const currentChampion = result.entries?.[0];
            if (ownsRun && payload.capture && currentChampion?.attemptId === payload.attemptId) {
                pendingChampionCapture = {
                    levelId: payload.levelId,
                    attemptId: payload.attemptId,
                    capture: payload.capture
                };
                recordingRetryCount = 0;
                await retryChampionRecording();
            }
        } catch {
            if (generation !== leaderboardGeneration) return;
            leaderboardState = {
                ...leaderboardState,
                status: 'unavailable',
                message: 'Ranking global no disponible. El récord personal quedó guardado localmente.'
            };
        }
    }
    async function gzipJson(value) {
        if (typeof CompressionStream === 'undefined')
            throw new Error('COMPRESSION_UNAVAILABLE');
        const stream = new Blob([JSON.stringify(value)], { type: 'application/json' })
            .stream().pipeThrough(new CompressionStream('gzip'));
        return new Uint8Array(await new Response(stream).arrayBuffer());
    }
    async function retryChampionRecording() {
        if (!pendingChampionCapture) {
            try {
                const saved = JSON.parse(localStorage.getItem('tunea-tu-combi:last-run') ?? 'null');
                if (saved?.attemptId && saved?.levelId && saved?.combiId === identity?.id) {
                    pendingChampionCapture = {
                        levelId: saved.levelId,
                        attemptId: saved.attemptId,
                        capture: saved
                    };
                }
            } catch {}
        }
        const pending = pendingChampionCapture;
        if (!pending || status !== 'ready' || recordingUploadBusy) return false;
        recordingUploadBusy = true;
        try {
            const bytes = await gzipJson(pending.capture);
            if (bytes.byteLength > 2 * 1024 * 1024) throw new Error('RECORDING_TOO_LARGE');
            const response = await fetch(`/api/v1${levelPath(pending.levelId)}/attempts/${encodeURIComponent(pending.attemptId)}/recording`, {
                method: 'PUT',
                credentials: 'same-origin',
                cache: 'no-store',
                headers: {
                    'content-type': 'application/vnd.tunea-tu-combi.run+gzip',
                    'x-combi-request': '1'
                },
                body: bytes
            });
            const result = await response.json();
            if (!response.ok) throw Object.assign(new Error(result.error), { status: response.status });
            if (pendingChampionCapture !== pending) return true;
            pendingChampionCapture = null;
            recordingRetryCount = 0;
            if (recordingRetryTimer) clearTimeout(recordingRetryTimer);
            recordingRetryTimer = null;
            const refreshed = await request(`${levelPath(pending.levelId)}/leaderboard`);
            leaderboardState = {
                ...leaderboardState,
                entries: refreshed.entries,
                championRunRef: refreshed.championRunRef ?? '',
                championRecording: refreshed.championRecording ?? { status: 'unavailable' },
                message: 'Puntaje y grabación campeona confirmados.'
            };
            return true;
        } catch (error) {
            if (pendingChampionCapture !== pending) return false;
            if (error.message === 'NOT_CURRENT_CHAMPION') pendingChampionCapture = null;
            leaderboardState = {
                ...leaderboardState,
                championRecording: { status: 'unavailable' },
                message: error.message === 'NOT_CURRENT_CHAMPION'
                    ? 'La marca ya fue superada; su carga tardía no reemplazó al campeón.'
                    : 'La marca quedó guardada, pero su fantasma no. Se puede reintentar la subida.'
            };
            if (pendingChampionCapture && recordingRetryCount < 3) {
                const delays = [2000, 5000, 15000];
                const delay = delays[recordingRetryCount++];
                recordingRetryTimer = setTimeout(
                    () => retryChampionRecording(), delay
                );
            }
            return false;
        } finally {
            recordingUploadBusy = false;
            if (pendingChampionCapture && pendingChampionCapture !== pending) {
                setTimeout(() => retryChampionRecording(), 0);
            }
        }
    }
    function cacheGhost(key, capture) {
        ghostCache.delete(key);
        ghostCache.set(key, capture);
        while (ghostCache.size > ghostCacheLimit) {
            ghostCache.delete(ghostCache.keys().next().value);
        }
    }
    async function prepareChampionGhost(levelId) {
        const generation = ++ghostGeneration;
        ghostState = { status: 'loading', message: '', capture: null, reference: null };
        try {
            if (!levelId) throw new Error('RECORDING_UNAVAILABLE');
            const board = await request(`${levelPath(levelId)}/leaderboard`);
            if (generation !== ghostGeneration) return false;
            const recording = board.championRecording;
            if (recording?.status !== 'ready' || !recording.attemptId || !recording.sha256) {
                ghostState = {
                    status: 'unavailable',
                    message: 'El campeón todavía no tiene recorrido disponible.',
                    capture: null,
                    reference: null
                };
                return false;
            }
            const reference = {
                levelId,
                attemptId: recording.attemptId,
                sha256: recording.sha256,
                formatVersion: recording.formatVersion,
                physicsVersion: recording.physicsVersion,
                rulesVersion: recording.rulesVersion
            };
            const cacheKey = `${levelId}|${recording.attemptId}|${recording.sha256}`;
            const cached = ghostCache.get(cacheKey);
            if (cached) {
                cacheGhost(cacheKey, cached);
                ghostState = { status: 'ready', message: '', capture: cached, reference };
            } else {
                ghostState = {
                    status: 'available',
                    message: 'Recorrido campeón disponible.',
                    capture: null,
                    reference
                };
            }
            return true;
        } catch (error) {
            if (generation !== ghostGeneration) return false;
            ghostState = {
                status: error.message === 'RECORDING_VERSION_MISMATCH'
                    ? 'incompatible'
                    : error.status === 404 || error.message === 'RECORDING_UNAVAILABLE'
                    ? 'unavailable'
                    : 'error',
                message: error.status === 404 || error.message === 'LEVEL_NOT_FOUND'
                    ? 'El nivel fue eliminado o ya no está disponible.'
                    : error.message === 'RECORDING_UNAVAILABLE'
                    ? 'El recorrido campeón no está disponible.'
                    : 'No se pudo consultar el recorrido campeón.',
                capture: null,
                reference: null
            };
            return false;
        }
    }
    async function loadPreparedChampionGhost() {
        const generation = ghostGeneration;
        const reference = ghostState.reference;
        if (ghostState.status === 'ready') return true;
        if (ghostState.status === 'loading-recording') return false;
        if (!reference || ghostState.status !== 'available') return false;
        ghostState = {
            ...ghostState,
            status: 'loading-recording',
            message: 'Descargando recorrido campeón...'
        };
        try {
            const response = await fetch(
                `/api/v1${levelPath(reference.levelId)}/attempts/${encodeURIComponent(reference.attemptId)}/recording` +
                    `?sha256=${encodeURIComponent(reference.sha256)}`,
                { credentials: 'same-origin' }
            );
            if (!response.ok) {
                const error = await response.json().catch(() => ({}));
                throw Object.assign(
                    new Error(error.error ?? 'RECORDING_UNAVAILABLE'),
                    { status: response.status }
                );
            }
            if (typeof DecompressionStream === 'undefined')
                throw new Error('DECOMPRESSION_UNAVAILABLE');
            const stream = response.body.pipeThrough(new DecompressionStream('gzip'));
            const capture = JSON.parse(await new Response(stream).text());
            if (generation !== ghostGeneration) return false;
            if (
                capture?.levelId !== reference.levelId ||
                capture?.attemptId !== reference.attemptId ||
                capture?.formatVersion !== 'run-v2' ||
                capture?.physicsVersion !== 'custom-physics-v1' ||
                capture?.rulesVersion !== 'score-v2'
            ) {
                ghostState = {
                    status: 'incompatible',
                    message: 'La versión del recorrido campeón no es compatible.',
                    capture: null,
                    reference
                };
                return false;
            }
            const cacheKey = `${reference.levelId}|${reference.attemptId}|${reference.sha256}`;
            cacheGhost(cacheKey, capture);
            ghostState = { status: 'ready', message: '', capture, reference };
            return true;
        } catch (error) {
            if (generation !== ghostGeneration) return false;
            ghostState = {
                status: error.message === 'RECORDING_VERSION_MISMATCH' ||
                    error.message === 'DECOMPRESSION_UNAVAILABLE'
                    ? 'incompatible'
                    : error.status === 404 || error.message === 'RECORDING_UNAVAILABLE'
                    ? 'unavailable'
                    : 'error',
                message: error.status === 404 || error.message === 'RECORDING_UNAVAILABLE'
                    ? 'El recorrido fijado ya no está disponible.'
                    : error.message === 'DECOMPRESSION_UNAVAILABLE'
                    ? 'Este navegador no puede descomprimir la grabación.'
                    : 'No se pudo descargar el recorrido campeón.',
                capture: null,
                reference
            };
            return false;
        }
    }
    function ghostSnapshot() { return ghostState; }
    function leaderboardSnapshot() {
        return leaderboardState;
    }
    const publicationMessages = {
        ATTEMPT_ALREADY_PUBLISHED: 'Esta victoria ya fue usada para publicar.',
        DRAFT_ALREADY_PUBLISHED: 'Este borrador ya originó una publicación. Crea una copia desde Comunidad para editarlo y volver a publicarlo.',
        PUBLICATION_LIMIT: 'Ya tienes cinco publicaciones activas. Elimina una antes de publicar.',
        RETRY_PUBLICATION: 'Hubo otra publicación simultánea. Vuelve a intentarlo.'
    };
    async function loadCatalog(cursor = null, pushPrevious = false,
        generation = ++catalogGeneration) {
        catalogState.loading = true;
        catalogState.error = '';
        try {
            // "Propios" lists only your published levels.
            if (catalogState.mine && status !== 'ready') throw new Error('NO_IDENTITY');
            const search = `query=${encodeURIComponent(catalogState.query)}${cursor ? `&cursor=${encodeURIComponent(cursor)}` : ''}`;
            const result = await request(catalogState.mine
                ? `/me/levels?status=public&${search}`
                : `/levels?${search}`);
            if (generation !== catalogGeneration) return;
            if (pushPrevious) catalogState.previous.push(catalogState.currentCursor);
            catalogState.currentCursor = cursor;
            catalogState.nextCursor = result.cursor ?? null;
            catalogState.items = (result.items ?? []).map(level =>
                catalogState.mine ? { ...level, author: { id: identity.id, name: identity.name } } : level);
        } catch {
            if (generation === catalogGeneration)
                catalogState.error = catalogState.mine
                    ? 'No se pudieron cargar tus niveles.'
                    : 'No se pudo cargar el catálogo público.';
        } finally {
            if (generation === catalogGeneration) catalogState.loading = false;
        }
    }
    function searchCatalog(query) {
        if (catalogSearchTimer !== null) clearTimeout(catalogSearchTimer);
        catalogState.query = query.trim();
        catalogState.previous = [];
        catalogState.currentCursor = null;
        catalogState.nextCursor = null;
        catalogState.loading = true;
        catalogState.error = '';
        const generation = ++catalogGeneration;
        catalogSearchTimer = setTimeout(() => {
            catalogSearchTimer = null;
            loadCatalog(null, false, generation);
        }, 180);
    }
    async function publishDraft(payload) {
        publicationState = {
            ...publicationState,
            status: 'publishing',
            message: 'Subiendo nivel y grabación...'
        };
        try {
            if (status !== 'ready' || !identity?.id || payload.capture?.combiId !== identity.id)
                throw new Error('IDENTITY_CHANGED');
            const result = await request('/levels', 'POST', payload);
            if (result.status === 'retired') throw new Error('LEVEL_ALREADY_RETIRED');
            publicationState = {
                ...publicationState,
                status: 'published',
                message: `“${result.name}” ya está en el catálogo.`
            };
            await loadCatalog(null);
            return true;
        } catch (error) {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: error.message === 'IDENTITY_CHANGED'
                    ? 'La victoria pertenece a otra sesión. Completa el nivel con la identidad actual.'
                    : error.message === 'LEVEL_ALREADY_RETIRED'
                    ? 'Este nivel ya fue publicado y eliminado, así que no se puede publicar de nuevo.'
                    : publicationMessages[error.message] ?? 'No se pudo publicar. Comprueba la conexión e inténtalo otra vez.'
            };
            return false;
        }
    }
    async function playPublicLevel(id) {
        try {
            const level = await request(publicLevelPath(id));
            publicationRevision++;
            publicationState = {
                ...publicationState,
                revision: publicationRevision,
                play: level,
                message: ''
            };
        } catch {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: 'El nivel ya no está disponible o falló la conexión.'
            };
        }
    }
    async function copyPublication(id) {
        try {
            const copy = await request(`${publicLevelPath(id)}/copies`, 'POST');
            publicationRevision++;
            publicationState = {
                ...publicationState,
                revision: publicationRevision,
                copy,
                message: 'Copia creada como borrador. Debes completarla antes de publicarla.'
            };
        } catch {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: 'No se pudo crear la copia.'
            };
        }
    }
    async function retirePublication(id) {
        const accepted = window.confirm(
            '¿Eliminar este nivel? Se quita del catálogo y no se puede deshacer.'
        );
        if (!accepted) return;
        try {
            await request(publicLevelPath(id), 'DELETE');
            publicationState = {
                ...publicationState,
                status: 'idle',
                message: ''
            };
            await loadCatalog(catalogState.currentCursor);
        } catch {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: 'No se pudo eliminar el nivel.'
            };
        }
    }
    function publicationSnapshot() {
        return publicationState;
    }
    function catalogSnapshot() {
        return {
            loading: catalogState.loading,
            error: catalogState.error,
            hasPrevious: catalogState.previous.length > 0,
            hasNext: Boolean(catalogState.nextCursor),
            onlyMine: catalogState.mine,
            items: catalogState.items.map(level => ({
                id: level.id,
                name: level.name,
                author: level.author?.name || 'Combi sin nombre',
                mine: Boolean(identity?.id) && level.author?.id === identity.id
            }))
        };
    }
    function openCatalog() {
        if (catalogSearchTimer !== null) clearTimeout(catalogSearchTimer);
        catalogSearchTimer = null;
        catalogState.query = '';
        catalogState.mine = false;
        catalogState.previous = [];
        loadCatalog(null);
    }
    function showOnlyMine(mine) {
        // From C++ a bool arrives as 0 or 1.
        catalogState.mine = Boolean(mine);
        catalogState.previous = [];
        loadCatalog(null);
    }
    function nextCatalogPage() {
        if (catalogState.nextCursor && !catalogState.loading)
            loadCatalog(catalogState.nextCursor, true);
    }
    function previousCatalogPage() {
        if (catalogState.previous.length && !catalogState.loading)
            loadCatalog(catalogState.previous.pop());
    }
    function consumePublicationAction(action) {
        if (action === 'play') publicationState.play = null;
        if (action === 'copy') publicationState.copy = null;
    }
    function mount() {
        const panel = document.createElement('section');
        panel.classList.add('combi-matcha');
        panel.setAttribute('aria-label', 'Identidad de tu combi');
        panel.style.cssText = 'max-width:800px;margin:12px auto;padding:12px;box-sizing:border-box;font-size:15px;background:#eef4e7;color:#203021;text-align:left';
        const state = document.createElement('p');
        state.setAttribute('role', 'status');
        const form = document.createElement('form');
        const label = document.createElement('label');
        const input = document.createElement('input');
        input.type = 'text'; input.maxLength = 64; input.autocomplete = 'off';
        input.placeholder = 'Nombre de tu combi';
        input.setAttribute('aria-label', 'Nombre de tu combi');
        input.setAttribute('aria-describedby', 'combi-name-rules');
        label.append(input);
        const save = document.createElement('button'); save.type = 'submit';
        const retry = document.createElement('button'); retry.type = 'button'; retry.textContent = 'Reintentar';
        const rules = document.createElement('p'); rules.id = 'combi-name-rules';
        rules.textContent = 'De 3 a 24 letras, números, espacios o guiones.';
        rules.title = '3–24 caracteres: letras latinas (con tildes), números, espacios o guiones; empieza y termina con letra o número. Mayúsculas y espacios sobrantes no distinguen nombres. Unicode NFKC; las tildes sí distinguen.';
        const warning = document.createElement('p');
        warning.textContent = 'Tu identidad vive en este navegador: si borras las cookies, no se puede recuperar.';
        warning.title = 'Si borras la cookie o vence (1 año), crearás otra identidad y perderás acceso a la anterior desde este navegador. Sus niveles, grabaciones y nombre reservado permanecen. No hay recuperación.';
        const feedback = document.createElement('p'); feedback.setAttribute('aria-live', 'polite');
        form.append(label, save); panel.append(state, form, retry, rules, warning, feedback);
        const canvas = document.querySelector('canvas');
        const container = canvas?.parentElement;
        if (container?.parentElement) container.before(panel); else document.body.prepend(panel);
        for (const event of ['keydown', 'keyup', 'keypress']) panel.addEventListener(event, e => e.stopPropagation());
        let renderedName = null;
        updateUi = () => {
            const ready = status === 'ready';
            state.textContent = ready
                ? (identity.name ? 'Tu combi: ' + identity.name : 'Elige un nombre para tu combi.')
                : 'Sin conexión. Puedes jugar, pero no reservar un nombre ni publicar niveles.';
            // Online: the name form. Offline: only a retry button.
            form.hidden = rules.hidden = warning.hidden = !ready;
            retry.hidden = ready;
            save.textContent = identity?.name ? 'Cambiar nombre' : 'Reservar nombre';
            if (identity?.name !== renderedName) { input.value = identity?.name ?? ''; renderedName = identity?.name; }
            save.disabled = busy || !ready; retry.disabled = busy;
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
        retryChampionRecording,
        leaderboardSnapshot,
        prepareChampionGhost,
        loadPreparedChampionGhost,
        ghostSnapshot,
        publishDraft,
        catalogSnapshot,
        openCatalog,
        showOnlyMine,
        searchCatalog,
        nextCatalogPage,
        previousCatalogPage,
        playPublicLevel,
        copyPublication,
        retirePublication,
        publicationSnapshot,
        consumePublicationAction
    };
})();
