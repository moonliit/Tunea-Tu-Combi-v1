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
    let renderPublications = () => {};
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
        query: '',
        currentCursor: null,
        nextCursor: null,
        previous: [],
        loading: false,
        error: ''
    };
    const ownPublicationState = {
        items: [],
        activeCount: 0,
        limit: 5,
        currentCursor: null,
        nextCursor: null,
        previous: [],
        loading: false,
        error: ''
    };
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
        if (identity?.id !== next?.id) {
            epoch++;
            ownPublicationState.currentCursor = null;
            ownPublicationState.nextCursor = null;
            ownPublicationState.previous = [];
        }
        identity = next;
        status = next ? 'ready' : 'local';
    }
    async function request(path, method = 'GET', body) {
        const controller = new AbortController();
        const timer = setTimeout(
            () => controller.abort(),
            path === '/publications' ? 20000 : 8000
        );
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
        notice = messages[error.message] ?? 'Sin conexión confirmada. Puedes jugar localmente, pero no puedes reservar ni publicar.';
        ownPublicationState.items = [];
        ownPublicationState.error = 'Sin conexión: la gestión de publicaciones no está disponible.';
        renderPublications();
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
                loadOwnPublications();
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
            loadCatalog(catalogState.currentCursor);
            loadOwnPublications();
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
                championRecording: result.championRecording ?? { status: 'unavailable' },
                message: ownsRun
                    ? 'Puntaje recalculado por el servidor. La física no fue verificada.'
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
            const response = await fetch(`/api/recordings/${encodeURIComponent(pending.attemptId)}`, {
                method: 'PUT',
                credentials: 'same-origin',
                cache: 'no-store',
                headers: {
                    'content-type': 'application/vnd.tunea-tu-combi.run+gzip',
                    'x-combi-request': '1',
                    'x-combi-level': pending.levelId
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
            const refreshed = await request(`/leaderboard?levelId=${encodeURIComponent(pending.levelId)}`);
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
    async function loadChampionRecording(levelId) {
        const generation = ++ghostGeneration;
        ghostState = { status: 'loading', message: '', capture: null };
        try {
            const response = await fetch(`/api/recordings?levelId=${encodeURIComponent(levelId)}`, {
                credentials: 'same-origin', cache: 'no-store'
            });
            if (!response.ok) {
                const error = await response.json().catch(() => ({}));
                throw new Error(error.error ?? 'RECORDING_UNAVAILABLE');
            }
            if (typeof DecompressionStream === 'undefined') throw new Error('DECOMPRESSION_UNAVAILABLE');
            const stream = response.body.pipeThrough(new DecompressionStream('gzip'));
            const capture = JSON.parse(await new Response(stream).text());
            if (generation !== ghostGeneration) return false;
            ghostState = { status: 'ready', message: '', capture };
            return true;
        } catch (error) {
            if (generation !== ghostGeneration) return false;
            ghostState = {
                status: 'unavailable',
                message: 'El recorrido campeón no está disponible.',
                capture: null
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
        DRAFT_CHANGED_AFTER_WIN: 'El borrador cambió después de la victoria. Complétalo otra vez.',
        DRAFT_ALREADY_PUBLISHED: 'Este borrador ya originó una publicación. Crea una copia desde “Mis publicaciones” para editar y volver a publicar.',
        INVALID_CAPTURE: 'La grabación no cumple los límites de publicación.',
        INVALID_GOAL_WINDOW: 'La grabación no confirma los tres segundos del chofer.',
        INVALID_LEVEL: 'El nivel supera los límites permitidos o tiene datos inválidos.',
        INVALID_LEVEL_NAME: 'Usa un nombre de nivel de 3 a 40 caracteres.',
        PUBLICATION_LIMIT: 'Ya tienes cinco publicaciones activas. Retira una antes de publicar.',
        RETRY_PUBLICATION: 'Hubo otra publicación simultánea. Vuelve a intentarlo.'
    };
    async function loadCatalog(cursor = null, pushPrevious = false) {
        catalogState.loading = true;
        catalogState.error = '';
        renderPublications();
        try {
            const query = encodeURIComponent(catalogState.query);
            const cursorValue = cursor ? `&cursor=${encodeURIComponent(cursor)}` : '';
            const result = await request(`/catalog?query=${query}${cursorValue}`);
            if (pushPrevious) catalogState.previous.push(catalogState.currentCursor);
            catalogState.currentCursor = cursor;
            catalogState.nextCursor = result.cursor ?? null;
            catalogState.items = result.items ?? [];
        } catch {
            catalogState.error = 'No se pudo cargar el catálogo público.';
        } finally {
            catalogState.loading = false;
            renderPublications();
        }
    }
    async function loadOwnPublications(cursor = null, pushPrevious = false) {
        if (status !== 'ready') {
            ownPublicationState.items = [];
            ownPublicationState.error = 'Conecta una identidad para gestionar tus publicaciones.';
            renderPublications();
            return;
        }
        ownPublicationState.loading = true;
        ownPublicationState.error = '';
        renderPublications();
        try {
            const cursorValue = cursor ? `?cursor=${encodeURIComponent(cursor)}` : '';
            const result = await request(`/publications/mine${cursorValue}`);
            if (pushPrevious) ownPublicationState.previous.push(ownPublicationState.currentCursor);
            ownPublicationState.currentCursor = cursor;
            ownPublicationState.nextCursor = result.cursor ?? null;
            ownPublicationState.items = result.items ?? [];
            ownPublicationState.activeCount = result.activeCount ?? 0;
            ownPublicationState.limit = result.limit ?? 5;
        } catch {
            ownPublicationState.error = 'No se pudieron cargar tus publicaciones.';
        } finally {
            ownPublicationState.loading = false;
            renderPublications();
        }
    }
    async function publishDraft(payload) {
        publicationState = {
            ...publicationState,
            status: 'publishing',
            message: 'Subiendo nivel y grabación...'
        };
        renderPublications();
        try {
            if (status !== 'ready' || !identity?.id || payload.capture?.combiId !== identity.id)
                throw new Error('IDENTITY_CHANGED');
            const result = await request('/publications', 'POST', payload);
            if (result.status === 'retired') throw new Error('LEVEL_ALREADY_RETIRED');
            publicationState = {
                ...publicationState,
                status: 'published',
                message: `“${result.name}” ya está en el catálogo.`
            };
            await Promise.all([loadCatalog(null), loadOwnPublications()]);
            return true;
        } catch (error) {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: error.message === 'IDENTITY_CHANGED'
                    ? 'La victoria pertenece a otra sesión. Completa el nivel con la identidad actual.'
                    : error.message === 'LEVEL_ALREADY_RETIRED'
                    ? 'Esta publicación fue retirada y no puede reactivarse. Crea una copia y complétala otra vez.'
                    : publicationMessages[error.message] ?? 'No se pudo publicar. Comprueba la conexión e inténtalo otra vez.'
            };
            renderPublications();
            return false;
        }
    }
    async function playPublicLevel(id) {
        try {
            const level = await request(`/levels/${encodeURIComponent(id)}`);
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
        renderPublications();
    }
    async function copyPublication(id) {
        try {
            const copy = await request(`/publications/${encodeURIComponent(id)}/copy`, 'POST', {});
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
        renderPublications();
    }
    async function retirePublication(id) {
        try {
            await request(`/publications/${encodeURIComponent(id)}/retire`, 'POST', {});
            publicationState = {
                ...publicationState,
                status: 'idle',
                message: 'Publicación retirada. El cupo quedó libre.'
            };
            await Promise.all([loadCatalog(null), loadOwnPublications()]);
        } catch {
            publicationState = {
                ...publicationState,
                status: 'error',
                message: 'No se pudo retirar la publicación.'
            };
            renderPublications();
        }
    }
    function publicationSnapshot() {
        return publicationState;
    }
    function consumePublicationAction(action) {
        if (action === 'play') publicationState.play = null;
        if (action === 'copy') publicationState.copy = null;
    }
    function mountPublications(container) {
        const panel = document.createElement('details');
        panel.style.cssText = 'max-width:800px;margin:12px auto;padding:12px;box-sizing:border-box;font:15px sans-serif;background:#f5f1e8;color:#30281f;text-align:left';
        const summary = document.createElement('summary');
        summary.textContent = 'Catálogo público y mis publicaciones';
        summary.style.cssText = 'font-weight:bold;cursor:pointer';
        const search = document.createElement('form');
        search.style.cssText = 'display:flex;gap:8px;margin:12px 0 4px';
        const searchInput = document.createElement('input');
        searchInput.placeholder = 'Nombre del nivel o autor';
        searchInput.maxLength = 40;
        searchInput.style.flex = '1';
        const searchButton = document.createElement('button');
        searchButton.type = 'submit';
        searchButton.textContent = 'Buscar';
        search.append(searchInput, searchButton);
        const scope = document.createElement('p');
        scope.textContent = 'Orden: publicaciones recientes. La búsqueda revisa como máximo las 100 más recientes. El servidor valida la estructura y la ventana registrada del chofer; no verifica la física ni evita trampas.';
        scope.style.cssText = 'margin:4px 0 10px;color:#665c50;font-size:13px';
        const catalogTitle = document.createElement('h3');
        catalogTitle.textContent = 'Niveles públicos';
        const catalogList = document.createElement('div');
        const navigation = document.createElement('div');
        navigation.style.cssText = 'display:flex;gap:8px;margin:8px 0 18px';
        const previous = document.createElement('button');
        previous.type = 'button';
        previous.textContent = 'Anterior';
        const next = document.createElement('button');
        next.type = 'button';
        next.textContent = 'Siguiente';
        navigation.append(previous, next);
        const ownTitle = document.createElement('h3');
        const ownList = document.createElement('div');
        const ownNavigation = document.createElement('div');
        ownNavigation.style.cssText = 'display:flex;gap:8px;margin:8px 0';
        const ownPrevious = document.createElement('button');
        ownPrevious.type = 'button';
        ownPrevious.textContent = 'Anteriores propias';
        const ownNext = document.createElement('button');
        ownNext.type = 'button';
        ownNext.textContent = 'Más publicaciones propias';
        ownNavigation.append(ownPrevious, ownNext);
        const feedback = document.createElement('p');
        feedback.setAttribute('aria-live', 'polite');
        panel.append(
            summary, search, scope, catalogTitle, catalogList, navigation,
            ownTitle, ownList, ownNavigation, feedback
        );
        container.before(panel);
        for (const event of ['keydown', 'keyup', 'keypress'])
            panel.addEventListener(event, (value) => value.stopPropagation());

        const row = (title, detail, actions) => {
            const item = document.createElement('div');
            item.style.cssText = 'display:flex;align-items:center;gap:8px;border-top:1px solid #d7cdbf;padding:8px 0';
            const text = document.createElement('div');
            text.style.flex = '1';
            const strong = document.createElement('strong');
            strong.textContent = title;
            const small = document.createElement('div');
            small.textContent = detail;
            small.style.cssText = 'font-size:13px;color:#665c50';
            text.append(strong, small);
            item.append(text, ...actions);
            return item;
        };
        const actionButton = (label, handler, disabled = false) => {
            const button = document.createElement('button');
            button.type = 'button';
            button.textContent = label;
            button.disabled = disabled;
            button.addEventListener('click', handler);
            return button;
        };
        renderPublications = () => {
            catalogList.replaceChildren();
            if (catalogState.loading) {
                catalogList.textContent = 'Cargando catálogo...';
            } else if (catalogState.error) {
                catalogList.textContent = catalogState.error;
            } else if (!catalogState.items.length) {
                catalogList.textContent = catalogState.query
                    ? 'No hay coincidencias dentro de las 100 publicaciones más recientes.'
                    : 'Todavía no hay niveles publicados.';
            } else {
                for (const level of catalogState.items) {
                    const author = level.author?.name || 'Combi sin nombre';
                    const best = level.best
                        ? `Mejor: ${level.best.total} puntos · ${level.best.driverTimeMs} ms`
                        : 'Sin marca disponible';
                    catalogList.append(row(
                        level.name,
                        `Por ${author} · ${best}`,
                        [actionButton('Jugar', () => playPublicLevel(level.id))]
                    ));
                }
            }
            previous.disabled = catalogState.loading || !catalogState.previous.length;
            next.disabled = catalogState.loading || !catalogState.nextCursor;

            ownTitle.textContent = `Mis publicaciones (${ownPublicationState.activeCount}/${ownPublicationState.limit} activas)`;
            ownList.replaceChildren();
            if (ownPublicationState.loading) {
                ownList.textContent = 'Cargando publicaciones...';
            } else if (ownPublicationState.error) {
                ownList.textContent = ownPublicationState.error;
            } else if (!ownPublicationState.items.length) {
                ownList.textContent = 'Todavía no tienes publicaciones.';
            } else {
                for (const level of ownPublicationState.items) {
                    const active = level.status === 'public';
                    const actions = [actionButton('Crear copia', () => copyPublication(level.id))];
                    if (active) {
                        actions.push(actionButton('Retirar', () => {
                            const accepted = window.confirm(
                                'Retirar quita el nivel del catálogo y libera un cupo. No puede reactivarse; sus puntuaciones se conservan y la grabación se elimina tras 24 horas. Las partidas en curso podrán terminar localmente, pero sus resultados serán rechazados. ¿Continuar?'
                            );
                            if (accepted) retirePublication(level.id);
                        }));
                    }
                    ownList.append(row(
                        level.name,
                        active ? 'Publicación activa' : 'Publicación retirada, no se puede reactivar',
                        actions
                    ));
                }
            }
            ownPrevious.disabled = ownPublicationState.loading ||
                !ownPublicationState.previous.length;
            ownNext.disabled = ownPublicationState.loading ||
                !ownPublicationState.nextCursor;
            feedback.textContent = publicationState.message;
        };
        search.addEventListener('submit', (event) => {
            event.preventDefault();
            catalogState.query = searchInput.value.trim();
            catalogState.previous = [];
            loadCatalog(null);
        });
        previous.addEventListener('click', () => {
            if (!catalogState.previous.length) return;
            loadCatalog(catalogState.previous.pop());
        });
        next.addEventListener('click', () => {
            if (catalogState.nextCursor) loadCatalog(catalogState.nextCursor, true);
        });
        ownPrevious.addEventListener('click', () => {
            if (!ownPublicationState.previous.length) return;
            loadOwnPublications(ownPublicationState.previous.pop());
        });
        ownNext.addEventListener('click', () => {
            if (ownPublicationState.nextCursor)
                loadOwnPublications(ownPublicationState.nextCursor, true);
        });
        renderPublications();
        loadCatalog(null);
        loadOwnPublications();
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
        if (container?.parentElement) mountPublications(container);
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
        retryChampionRecording,
        leaderboardSnapshot,
        loadChampionRecording,
        ghostSnapshot,
        publishDraft,
        publicationSnapshot,
        consumePublicationAction
    };
})();
