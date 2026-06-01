#ifndef WEB_UI_HTML_H
#define WEB_UI_HTML_H

const char kWebUiHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RaceChrono DIY BLE Device</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f4efe6;
      --panel: rgba(255, 250, 244, 0.92);
      --panel-border: rgba(73, 57, 37, 0.12);
      --text: #2d2419;
      --muted: #75624d;
      --accent: #0f766e;
      --accent-dark: #115e59;
      --danger: #b42318;
      --shadow: 0 24px 60px rgba(73, 57, 37, 0.12);
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      font-family: "Segoe UI", "Trebuchet MS", sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at top left, rgba(15, 118, 110, 0.12), transparent 32%),
        radial-gradient(circle at top right, rgba(214, 160, 94, 0.18), transparent 28%),
        linear-gradient(180deg, #fbf8f3, var(--bg));
      min-height: 100vh;
    }

    .page {
      width: min(1200px, calc(100vw - 24px));
      margin: 0 auto;
      padding: 24px 0 40px;
    }

    .hero {
      background: linear-gradient(135deg, rgba(15, 118, 110, 0.92), rgba(17, 94, 89, 0.82));
      color: #f8fafc;
      border-radius: 24px;
      box-shadow: var(--shadow);
      padding: 28px;
      margin-bottom: 20px;
    }

    .hero h1 {
      margin: 0 0 10px;
      font-size: clamp(1.8rem, 3vw, 2.7rem);
      letter-spacing: 0.02em;
    }

    .hero p {
      margin: 0;
      max-width: 70ch;
      line-height: 1.5;
      color: rgba(248, 250, 252, 0.88);
    }

    .grid {
      display: grid;
      gap: 16px;
    }

    .grid.top {
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      margin-bottom: 16px;
    }

    .card {
      background: var(--panel);
      border: 1px solid var(--panel-border);
      border-radius: 20px;
      box-shadow: var(--shadow);
      padding: 20px;
      backdrop-filter: blur(10px);
    }

    .card h2 {
      margin: 0 0 14px;
      font-size: 1.05rem;
      letter-spacing: 0.04em;
      text-transform: uppercase;
      color: var(--muted);
    }

    .inline-row {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      align-items: center;
    }

    label {
      display: grid;
      gap: 6px;
      font-size: 0.95rem;
      color: var(--muted);
      width: 100%;
    }

    input,
    select,
    button {
      border-radius: 12px;
      border: 1px solid rgba(73, 57, 37, 0.18);
      padding: 10px 12px;
      font: inherit;
    }

    input,
    select {
      background: #fff;
      color: var(--text);
      width: 100%;
    }

    input.rateLimit {
      width: 6.5rem;
      min-width: 6.5rem;
    }

    button {
      background: var(--accent);
      color: #fff;
      cursor: pointer;
      transition: transform 120ms ease, background 120ms ease;
    }

    button:hover {
      transform: translateY(-1px);
      background: var(--accent-dark);
    }

    button.secondary {
      background: #fff;
      color: var(--text);
    }

    .status {
      min-height: 1.25rem;
      color: var(--muted);
      font-size: 0.92rem;
    }

    .status.error {
      color: var(--danger);
    }

    .meta-list {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    }

    .meta-box {
      padding: 14px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(73, 57, 37, 0.08);
    }

    .meta-box .label {
      display: block;
      font-size: 0.82rem;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.06em;
      margin-bottom: 4px;
    }

    .meta-box .value {
      font-size: 1.05rem;
      font-weight: 600;
    }

    .table-shell {
      overflow: hidden;
    }

    .table-scroll {
      overflow-x: auto;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      min-width: 900px;
    }

    th,
    td {
      padding: 12px 10px;
      border-bottom: 1px solid rgba(73, 57, 37, 0.08);
      text-align: left;
      vertical-align: middle;
    }

    th {
      font-size: 0.82rem;
      letter-spacing: 0.05em;
      text-transform: uppercase;
      color: var(--muted);
    }

    td code {
      font-family: "Consolas", "Cascadia Code", monospace;
      font-size: 0.92rem;
      background: rgba(15, 118, 110, 0.08);
      padding: 3px 6px;
      border-radius: 8px;
    }

    .data-bytes {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
    }

    .data-byte {
      min-width: 2.2rem;
      text-align: center;
      font-family: "Consolas", "Cascadia Code", monospace;
      padding: 6px 8px;
      border-radius: 10px;
      background: rgba(15, 118, 110, 0.08);
    }

    .row-actions {
      display: flex;
      gap: 8px;
      align-items: center;
    }

    .can-id-cell {
      display: flex;
      gap: 8px;
      align-items: center;
    }

    .row-toggle {
      min-width: 2rem;
      width: 2rem;
      height: 2rem;
      padding: 0;
      border-radius: 10px;
      background: rgba(15, 118, 110, 0.08);
      color: var(--accent-dark);
      border-color: rgba(15, 118, 110, 0.22);
      font-size: 1rem;
      line-height: 1;
    }

    .row-toggle:hover {
      background: rgba(15, 118, 110, 0.16);
      color: var(--accent-dark);
    }

    .can-row.is-expanded td {
      border-bottom-color: transparent;
    }

    .can-detail-row td {
      padding: 0 10px 16px;
      border-bottom: 1px solid rgba(73, 57, 37, 0.08);
    }

    .can-detail-shell {
      margin-left: 38px;
      padding: 16px;
      border-radius: 18px;
      border: 1px solid rgba(73, 57, 37, 0.08);
      background: rgba(255, 255, 255, 0.78);
    }

    .can-detail-header {
      display: flex;
      justify-content: space-between;
      gap: 10px;
      align-items: flex-start;
      margin-bottom: 12px;
    }

    .can-detail-title {
      font-size: 1rem;
      font-weight: 600;
    }

    .can-detail-subtitle {
      margin-top: 4px;
      font-size: 0.88rem;
      color: var(--muted);
    }

    .can-detail-raw {
      margin-bottom: 12px;
    }

    .detail-label {
      margin-bottom: 8px;
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--muted);
    }

    .detail-note {
      margin: 0;
      color: var(--muted);
      line-height: 1.5;
    }

    .signal-grid {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
    }

    .signal-card {
      padding: 12px;
      border-radius: 14px;
      border: 1px solid rgba(73, 57, 37, 0.08);
      background: rgba(15, 118, 110, 0.05);
    }

    .signal-name {
      font-size: 0.78rem;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.05em;
      margin-bottom: 6px;
    }

    .signal-value {
      font-size: 1.12rem;
      font-weight: 600;
      margin-bottom: 6px;
    }

    .signal-meta {
      font-size: 0.84rem;
      color: var(--muted);
      line-height: 1.4;
    }

    .muted-text {
      color: var(--muted);
    }

    .row-actions input[type="number"] {
      width: 100px;
    }

    .empty {
      padding: 22px 0 4px;
      color: var(--muted);
    }

    @media (max-width: 720px) {
      .page {
        width: min(100vw - 16px, 1200px);
        padding-top: 16px;
      }

      .hero,
      .card {
        border-radius: 18px;
      }
    }
  </style>
</head>
<body>
  <div class="page">
    <section class="hero">
      <h1>RaceChrono DIY BLE Device</h1>
      <p>
        Laufzeit-Konfiguration fuer CAN-Baudrate, ID-Filter, DBC-Namen und
        Sende-Limits direkt auf dem ESP32. Aenderungen werden sofort im Flash gespeichert.
      </p>
    </section>

    <section class="grid top">
      <article class="card">
        <h2>CAN Setup</h2>
        <div class="inline-row">
          <label>
            CAN Baudrate
            <select id="baudRate">
              <option value="100000">100 kbit/s</option>
              <option value="125000">125 kbit/s</option>
              <option value="200000">200 kbit/s</option>
              <option value="250000">250 kbit/s</option>
              <option value="500000">500 kbit/s</option>
              <option value="1000000">1 Mbit/s</option>
            </select>
          </label>
          <button id="saveCanConfig">Speichern</button>
        </div>
        <p class="status" id="configStatus"></p>
      </article>

      <article class="card">
        <h2>DBC Upload</h2>
        <form id="dbcForm" class="grid">
          <label>
            DBC Datei
            <input type="file" id="dbcFile" accept=".dbc">
          </label>
          <div class="inline-row">
            <button type="submit">Hochladen und speichern</button>
          </div>
        </form>
        <p class="status" id="dbcStatus"></p>
      </article>
    </section>

    <section class="card">
      <h2>Geraetestatus</h2>
      <div class="meta-list" id="deviceMeta"></div>
    </section>

    <section class="card table-shell">
      <h2>Aktuelle CAN IDs</h2>
      <p class="status">`Limit Hz` = zusaetzliche Obergrenze fuer RaceChrono. `0` bedeutet unbegrenzt, `Ignorieren` blockiert die ID komplett.</p>
      <div class="table-scroll">
        <table>
          <thead>
            <tr>
              <th>CAN ID</th>
              <th>Name</th>
              <th>Aktuelle Frequenz</th>
              <th>Zuletzt gesehen</th>
              <th>Letzte 8 Bytes</th>
              <th>Ignorieren</th>
              <th>Limit Hz</th>
              <th>Aktion</th>
            </tr>
          </thead>
          <tbody id="canTableBody"></tbody>
        </table>
      </div>
      <div class="empty" id="emptyState" hidden>Es wurden noch keine CAN IDs empfangen.</div>
    </section>
  </div>

  <script>
    var state = {
      snapshot: null,
      stats: [],
      refreshTimer: null,
      isRefreshingStats: false,
      rowsById: {},
      expandedCanIds: {},
      canDetailsById: {},
      detailRequests: {}
    };

    var fallbackBaudRates = [100000, 125000, 200000, 250000, 500000, 1000000];

    function safeCall(callback, argument) {
      if (typeof callback === 'function') {
        callback(argument);
      }
    }

    function escapeHtml(value) {
      var text = String(value == null ? '' : value);
      return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
    }

    function normalizeStateSnapshot(snapshot) {
      var normalized = snapshot && typeof snapshot === 'object' ? snapshot : {};
      if (!normalized.accessPoint || typeof normalized.accessPoint !== 'object') {
        normalized.accessPoint = {};
      }
      if (!normalized.can || typeof normalized.can !== 'object') {
        normalized.can = {};
      }
      if (!normalized.dbc || typeof normalized.dbc !== 'object') {
        normalized.dbc = {};
      }
      if (!Array.isArray(normalized.can.supportedBaudRates) || !normalized.can.supportedBaudRates.length) {
        normalized.can.supportedBaudRates = fallbackBaudRates.slice(0);
      }

      normalized.accessPoint.ssid = normalized.accessPoint.ssid || '';
      normalized.accessPoint.ip = normalized.accessPoint.ip || '';
      normalized.can.baudRate = Number(normalized.can.baudRate || normalized.can.supportedBaudRates[0] || fallbackBaudRates[0]);
      normalized.dbc.fileName = normalized.dbc.fileName || '';
      normalized.dbc.messageCount = Number(normalized.dbc.messageCount || 0);
      normalized.observedCanIdCount = Number(normalized.observedCanIdCount || 0);
      return normalized;
    }

    function normalizeStatsResponse(response) {
      var normalized = response && typeof response === 'object' ? response : {};
      if (!Array.isArray(normalized.stats)) {
        return [];
      }
      return normalized.stats;
    }

    function normalizeCanDetailsResponse(response) {
      var normalized = response && typeof response === 'object' ? response : {};
      if (!Array.isArray(normalized.data)) {
        normalized.data = [];
      }
      if (!Array.isArray(normalized.signals)) {
        normalized.signals = [];
      }
      normalized.canId = Number(normalized.canId || 0);
      normalized.length = Number(normalized.length || 0);
      normalized.name = normalized.name || '';
      normalized.note = normalized.note || '';
      normalized.hasDbcFile = !!normalized.hasDbcFile;
      normalized.dbcMatched = !!normalized.dbcMatched;
      return normalized;
    }

    function setStatus(elementId, message, isError) {
      var element = document.getElementById(elementId);
      var className;
      if (!element) {
        return;
      }

      element.textContent = message || '';
      className = element.className || '';
      className = className.replace(/\berror\b/g, '').replace(/\s+/g, ' ').replace(/^\s+|\s+$/g, '');
      if (isError) {
        className = className ? className + ' error' : 'error';
      }
      element.className = className;
    }

    function padHexByte(value) {
      var hex = Number(value).toString(16).toUpperCase();
      return hex.length < 2 ? '0' + hex : hex;
    }

    function formatHexId(canId) {
      return '0x' + Number(canId).toString(16).toUpperCase();
    }

    function formatBaudRate(baudRate) {
      if (baudRate >= 1000000) {
        return '1 Mbit/s';
      }
      return (baudRate / 1000) + ' kbit/s';
    }

    function formatFrequency(milliHz) {
      var hz = Number(milliHz) / 1000;
      if (hz >= 100) {
        return hz.toFixed(0) + ' Hz';
      }
      if (hz >= 10) {
        return hz.toFixed(1) + ' Hz';
      }
      return hz.toFixed(2) + ' Hz';
    }

    function formatAge(milliseconds) {
      if (milliseconds < 1000) {
        return milliseconds + ' ms';
      }
      return (milliseconds / 1000).toFixed(1) + ' s';
    }

    function renderDataBytes(entry) {
      var bytes = [];
      var index;
      for (index = 0; index < 8; index += 1) {
        if (index < entry.length) {
          bytes.push('<span class="data-byte">' + padHexByte(entry.data[index]) + '</span>');
        } else {
          bytes.push('<span class="data-byte">--</span>');
        }
      }
      return '<div class="data-bytes">' + bytes.join('') + '</div>';
    }

    function renderMeta(snapshot) {
      var metaContainer = document.getElementById('deviceMeta');
      var observedCount = state.stats.length;
      if (!observedCount && snapshot && snapshot.observedCanIdCount) {
        observedCount = Number(snapshot.observedCanIdCount);
      }
      var items = [
        { label: 'Access Point', value: snapshot.accessPoint.ssid },
        { label: 'IP Adresse', value: snapshot.accessPoint.ip },
        { label: 'CAN Baudrate', value: formatBaudRate(snapshot.can.baudRate) },
        { label: 'DBC Datei', value: snapshot.dbc.fileName || 'Keine' },
        { label: 'DBC Nachrichten', value: String(snapshot.dbc.messageCount) },
        { label: 'Empfangene IDs', value: String(observedCount) }
      ];
      var html = '';
      var index;

      for (index = 0; index < items.length; index += 1) {
        html += '<div class="meta-box">' +
          '<span class="label">' + escapeHtml(items[index].label) + '</span>' +
          '<span class="value">' + escapeHtml(items[index].value) + '</span>' +
          '</div>';
      }

      metaContainer.innerHTML = html;
    }

    function renderBaudRates(snapshot) {
      var select = document.getElementById('baudRate');
      var canState = snapshot && snapshot.can ? snapshot.can : {};
      var baudRates = Array.isArray(canState.supportedBaudRates) && canState.supportedBaudRates.length ?
        canState.supportedBaudRates : fallbackBaudRates;
      var currentBaudRate = Number(canState.baudRate || baudRates[0]);
      var html = '';
      var index;
      var baudRate;
      var selected;

      for (index = 0; index < baudRates.length; index += 1) {
        baudRate = baudRates[index];
        selected = baudRate === currentBaudRate ? ' selected' : '';
        html += '<option value="' + baudRate + '"' + selected + '>' + formatBaudRate(baudRate) + '</option>';
      }

      select.innerHTML = html;
    }

    function setHidden(element, isHidden) {
      if ('hidden' in element) {
        element.hidden = isHidden;
      } else {
        element.style.display = isHidden ? 'none' : '';
      }
    }

    function formatSignalValue(value) {
      var numeric = Number(value);
      var absolute;
      if (!isFinite(numeric)) {
        return '-';
      }

      absolute = Math.abs(numeric);
      if (absolute >= 1000) {
        return numeric.toFixed(0);
      }
      if (absolute >= 100) {
        return numeric.toFixed(1);
      }
      if (absolute >= 10) {
        return numeric.toFixed(2);
      }
      if (absolute >= 1) {
        return numeric.toFixed(3);
      }
      return numeric.toFixed(4);
    }

    function getCanKey(canId) {
      return String(Number(canId));
    }

    function findStatByCanId(canId) {
      var numericCanId = Number(canId);
      var index;
      for (index = 0; index < state.stats.length; index += 1) {
        if (Number(state.stats[index].id) === numericCanId) {
          return state.stats[index];
        }
      }
      return null;
    }

    function insertRowPair(pair, canId) {
      var body = document.getElementById('canTableBody');
      var existingRows = body.querySelectorAll('tr.can-row');
      var index;

      for (index = 0; index < existingRows.length; index += 1) {
        if (Number(existingRows[index].getAttribute('data-can-id')) > Number(canId)) {
          body.insertBefore(pair.mainRow, existingRows[index]);
          body.insertBefore(pair.detailRow, existingRows[index]);
          return;
        }
      }

      body.appendChild(pair.mainRow);
      body.appendChild(pair.detailRow);
    }

    function createCanRowPair(canId) {
      var mainRow = document.createElement('tr');
      var detailRow = document.createElement('tr');
      var toggleButton;
      var saveButton;
      var key = getCanKey(canId);

      mainRow.className = 'can-row';
      mainRow.setAttribute('data-can-id', key);
      mainRow.innerHTML = '' +
        '<td class="can-id-cell">' +
        '<button class="row-toggle secondary" type="button" aria-expanded="false" aria-label="DBC Details umschalten">▸</button>' +
        '<code class="cell-id"></code>' +
        '</td>' +
        '<td class="cell-name"></td>' +
        '<td class="cell-frequency"></td>' +
        '<td class="cell-age"></td>' +
        '<td class="cell-data"></td>' +
        '<td><input class="ignore" type="checkbox"></td>' +
        '<td><input class="rateLimit" type="number" min="0" max="2000" step="1"></td>' +
        '<td class="row-actions"><button class="secondary save-rule-button" type="button">Speichern</button></td>';

      detailRow.className = 'can-detail-row';
      detailRow.setAttribute('data-can-id', key);
      detailRow.hidden = true;
      detailRow.innerHTML = '' +
        '<td colspan="8">' +
        '<div class="can-detail-shell">' +
        '<div class="can-detail-header">' +
        '<div>' +
        '<div class="can-detail-title"></div>' +
        '<div class="can-detail-subtitle"></div>' +
        '</div>' +
        '</div>' +
        '<div class="can-detail-content"><p class="detail-note">Noch keine Details geladen.</p></div>' +
        '</div>' +
        '</td>';

      toggleButton = mainRow.querySelector('.row-toggle');
      saveButton = mainRow.querySelector('.save-rule-button');
      toggleButton.addEventListener('click', function() {
        toggleCanDetails(canId);
      });
      saveButton.addEventListener('click', function() {
        saveRule(canId);
      });

      return {
        mainRow: mainRow,
        detailRow: detailRow
      };
    }

    function ensureCanRowPair(canId) {
      var key = getCanKey(canId);
      if (!state.rowsById[key]) {
        state.rowsById[key] = createCanRowPair(canId);
        insertRowPair(state.rowsById[key], canId);
      }
      return state.rowsById[key];
    }

    function renderSignalCards(details) {
      var html = '';
      var index;
      var signal;
      var valueText;
      var metaText;

      html += '<div class="can-detail-raw">';
      html += '<div class="detail-label">Rohdaten</div>';
      html += renderDataBytes(details);
      html += '</div>';

      if (!details.signals.length) {
        html += '<p class="detail-note">' + escapeHtml(details.note || 'Keine dekodierbaren DBC Signale verfuegbar.') + '</p>';
        return html;
      }

      html += '<div class="signal-grid">';
      for (index = 0; index < details.signals.length; index += 1) {
        signal = details.signals[index];
        valueText = formatSignalValue(signal.physicalValue);
        if (signal.unit) {
          valueText += ' ' + signal.unit;
        }
        metaText = 'Rohwert ' + formatSignalValue(signal.rawValue) + ' | Bit ' + signal.startBit + ' | ' + signal.bitLength + ' Bit';
        metaText += signal.isLittleEndian ? ' | Intel' : ' | Motorola';
        metaText += signal.isSigned ? ' | signed' : ' | unsigned';

        html += '<article class="signal-card">';
        html += '<div class="signal-name">' + escapeHtml(signal.name) + '</div>';
        html += '<div class="signal-value">' + escapeHtml(valueText) + '</div>';
        html += '<div class="signal-meta">' + escapeHtml(metaText) + '</div>';
        html += '</article>';
      }
      html += '</div>';
      return html;
    }

    function updateCanDetailRow(canId) {
      var key = getCanKey(canId);
      var pair = state.rowsById[key];
      var expanded = !!state.expandedCanIds[key];
      var details = state.canDetailsById[key] || null;
      var statEntry = findStatByCanId(canId);
      var toggleButton;
      var titleElement;
      var subtitleElement;
      var contentElement;
      var displayName;

      if (!pair) {
        return;
      }

      toggleButton = pair.mainRow.querySelector('.row-toggle');
      toggleButton.textContent = expanded ? '▾' : '▸';
      toggleButton.setAttribute('aria-expanded', expanded ? 'true' : 'false');
      pair.mainRow.className = expanded ? 'can-row is-expanded' : 'can-row';
      pair.detailRow.hidden = !expanded;
      if (!expanded) {
        return;
      }

      titleElement = pair.detailRow.querySelector('.can-detail-title');
      subtitleElement = pair.detailRow.querySelector('.can-detail-subtitle');
      contentElement = pair.detailRow.querySelector('.can-detail-content');

      if (details && details.name) {
        displayName = details.name;
      } else if (statEntry && statEntry.name) {
        displayName = statEntry.name;
      } else {
        displayName = 'CAN ' + formatHexId(canId);
      }
      titleElement.textContent = displayName;

      if (!details) {
        subtitleElement.textContent = 'DBC Details werden geladen...';
        contentElement.innerHTML = '<p class="detail-note">Signale werden geladen...</p>';
        return;
      }

      if (details.dbcMatched) {
        subtitleElement.textContent = 'Dekodierte Signale aus der hochgeladenen DBC Datei.';
      } else if (details.hasDbcFile) {
        subtitleElement.textContent = 'Fuer diese ID gibt es keine passende Nachricht in der DBC Datei.';
      } else {
        subtitleElement.textContent = 'Es ist noch keine DBC Datei hochgeladen.';
      }

      contentElement.innerHTML = renderSignalCards(details);
    }

    function updateCanMainRow(entry) {
      var pair = ensureCanRowPair(entry.id);
      var nameCell = pair.mainRow.querySelector('.cell-name');
      var ignoreInput = pair.mainRow.querySelector('.ignore');
      var rateLimitInput = pair.mainRow.querySelector('.rateLimit');

      pair.mainRow.querySelector('.cell-id').textContent = formatHexId(entry.id);
      if (entry.name) {
        nameCell.textContent = entry.name;
      } else {
        nameCell.innerHTML = '<span class="muted-text">Unbenannt</span>';
      }
      pair.mainRow.querySelector('.cell-frequency').textContent = formatFrequency(entry.frequencyMilliHz);
      pair.mainRow.querySelector('.cell-age').textContent = formatAge(entry.lastSeenMsAgo);
      pair.mainRow.querySelector('.cell-data').innerHTML = renderDataBytes(entry);

      if (document.activeElement !== ignoreInput) {
        ignoreInput.checked = !!entry.ignored;
      }
      if (document.activeElement !== rateLimitInput) {
        rateLimitInput.value = String(Number(entry.rateLimitHz || 0));
      }

      updateCanDetailRow(entry.id);
    }

    function syncStatsTable(stats) {
      var emptyState = document.getElementById('emptyState');
      var activeIds = {};
      var key;
      var index;

      if (!stats.length) {
        for (key in state.rowsById) {
          if (state.rowsById.hasOwnProperty(key)) {
            state.rowsById[key].mainRow.parentNode.removeChild(state.rowsById[key].mainRow);
            state.rowsById[key].detailRow.parentNode.removeChild(state.rowsById[key].detailRow);
          }
        }
        state.rowsById = {};
        state.expandedCanIds = {};
        state.canDetailsById = {};
        setHidden(emptyState, false);
        return;
      }

      setHidden(emptyState, true);
      for (index = 0; index < stats.length; index += 1) {
        activeIds[getCanKey(stats[index].id)] = true;
        updateCanMainRow(stats[index]);
      }

      for (key in state.rowsById) {
        if (state.rowsById.hasOwnProperty(key) && !activeIds[key]) {
          state.rowsById[key].mainRow.parentNode.removeChild(state.rowsById[key].mainRow);
          state.rowsById[key].detailRow.parentNode.removeChild(state.rowsById[key].detailRow);
          delete state.rowsById[key];
          delete state.expandedCanIds[key];
          delete state.canDetailsById[key];
          delete state.detailRequests[key];
        }
      }
    }

    function render(snapshot) {
      renderMeta(snapshot);
      renderBaudRates(snapshot);
      syncStatsTable(state.stats);
    }

    function parseJsonResponse(request) {
      if (!request.responseText) {
        return {};
      }

      try {
        return JSON.parse(request.responseText);
      } catch (error) {
        return {};
      }
    }

    function fetchState(onSuccess, onError) {
      var request = new XMLHttpRequest();
      request.open('GET', '/api/state', true);
      request.setRequestHeader('Cache-Control', 'no-store');
      request.onreadystatechange = function() {
        if (request.readyState !== 4) {
          return;
        }

        if (request.status >= 200 && request.status < 300) {
          state.snapshot = normalizeStateSnapshot(parseJsonResponse(request));
          render(state.snapshot);
          safeCall(onSuccess);
          return;
        }

        safeCall(onError, new Error('Status konnte nicht geladen werden.'));
      };
      request.onerror = function() {
        safeCall(onError, new Error('Netzwerkfehler beim Laden des Status.'));
      };
      request.send(null);
    }

    function fetchStats(onSuccess, onError) {
      var request = new XMLHttpRequest();
      request.open('GET', '/api/stats', true);
      request.setRequestHeader('Cache-Control', 'no-store');
      request.onreadystatechange = function() {
        if (request.readyState !== 4) {
          return;
        }

        if (request.status >= 200 && request.status < 300) {
          state.stats = normalizeStatsResponse(parseJsonResponse(request));
          if (!state.snapshot) {
            state.snapshot = normalizeStateSnapshot({});
          }
          renderMeta(state.snapshot);
          syncStatsTable(state.stats);
          safeCall(onSuccess);
          return;
        }

        safeCall(onError, new Error('CAN Nachrichten konnten nicht geladen werden.'));
      };
      request.onerror = function() {
        safeCall(onError, new Error('Netzwerkfehler beim Laden der CAN Nachrichten.'));
      };
      request.send(null);
    }

    function fetchCanDetails(canId, onSuccess, onError) {
      var request = new XMLHttpRequest();
      request.open('GET', '/api/can-details?canId=' + encodeURIComponent(canId), true);
      request.setRequestHeader('Cache-Control', 'no-store');
      request.onreadystatechange = function() {
        var result;
        if (request.readyState !== 4) {
          return;
        }

        result = parseJsonResponse(request);
        if (request.status >= 200 && request.status < 300) {
          safeCall(onSuccess, normalizeCanDetailsResponse(result));
          return;
        }

        safeCall(onError, new Error(result.error || 'DBC Details konnten nicht geladen werden.'));
      };
      request.onerror = function() {
        safeCall(onError, new Error('Netzwerkfehler beim Laden der DBC Details.'));
      };
      request.send(null);
    }

    function postJson(url, payload, onSuccess, onError) {
      var request = new XMLHttpRequest();
      request.open('POST', url, true);
      request.setRequestHeader('Content-Type', 'application/json');
      request.onreadystatechange = function() {
        var result;
        if (request.readyState !== 4) {
          return;
        }

        result = parseJsonResponse(request);
        if (request.status >= 200 && request.status < 300) {
          onSuccess(result);
          return;
        }

        onError(new Error(result.error || 'Anfrage fehlgeschlagen.'));
      };
      request.onerror = function() {
        onError(new Error('Netzwerkfehler bei der Anfrage.'));
      };
      request.send(JSON.stringify(payload));
    }

    function refreshStats(onSuccess, onError) {
      if (state.isRefreshingStats) {
        return;
      }

      state.isRefreshingStats = true;
      fetchStats(function() {
        state.isRefreshingStats = false;
        safeCall(onSuccess);
      }, function(error) {
        state.isRefreshingStats = false;
        safeCall(onError, error);
      });
    }

    function refreshCanDetails(canId, onSuccess, onError) {
      var key = getCanKey(canId);
      if (state.detailRequests[key]) {
        return;
      }

      state.detailRequests[key] = true;
      fetchCanDetails(canId, function(details) {
        delete state.detailRequests[key];
        state.canDetailsById[key] = details;
        updateCanDetailRow(canId);
        safeCall(onSuccess, details);
      }, function(error) {
        delete state.detailRequests[key];
        state.canDetailsById[key] = normalizeCanDetailsResponse({
          canId: canId,
          note: error.message,
          hasDbcFile: false,
          dbcMatched: false,
          data: [],
          signals: []
        });
        updateCanDetailRow(canId);
        safeCall(onError, error);
      });
    }

    function refreshExpandedCanDetails() {
      var key;
      for (key in state.expandedCanIds) {
        if (state.expandedCanIds.hasOwnProperty(key) && state.expandedCanIds[key]) {
          refreshCanDetails(Number(key));
        }
      }
    }

    function refreshAll(onSuccess, onError) {
      fetchState(function() {
        refreshStats(function() {
          refreshExpandedCanDetails();
          safeCall(onSuccess);
        }, function(error) {
          safeCall(onError, error);
        });
      }, function(error) {
        safeCall(onError, error);
      });
    }

    function refreshAfterAction(statusId, successMessage) {
      refreshAll(function() {
        if (successMessage) {
          setStatus(statusId, successMessage, false);
        }
      }, function(error) {
        if (successMessage) {
          setStatus(statusId, successMessage + ' Status konnte nicht aktualisiert werden.', true);
          return;
        }
        setStatus(statusId, error.message, true);
      });
    }

    function saveCanConfig() {
      var baudRate = Number(document.getElementById('baudRate').value);
      postJson('/api/can', { baudRate: baudRate }, function(result) {
        refreshAfterAction('configStatus', result.message || 'CAN Konfiguration gespeichert.');
      }, function(error) {
        setStatus('configStatus', error.message, true);
      });
    }

    function saveRule(canId) {
      var row = document.querySelector('tr.can-row[data-can-id="' + canId + '"]');
      var ignored;
      var rateLimitHz;
      if (!row) {
        return;
      }

      ignored = row.querySelector('.ignore').checked;
      rateLimitHz = Number(row.querySelector('.rateLimit').value || 0);
      postJson('/api/rule', {
        canId: canId,
        ignored: ignored,
        rateLimitHz: rateLimitHz
      }, function(result) {
        refreshAfterAction('configStatus', result.message || ('Regel fuer ' + formatHexId(canId) + ' gespeichert.'));
      }, function(error) {
        setStatus('configStatus', error.message, true);
      });
    }

    function uploadDbc(event) {
      var fileInput = document.getElementById('dbcFile');
      var formData;
      var request;

      if (event && event.preventDefault) {
        event.preventDefault();
      }

      if (!fileInput.files || !fileInput.files.length) {
        setStatus('dbcStatus', 'Bitte zuerst eine DBC Datei auswaehlen.', true);
        return false;
      }

      setStatus('dbcStatus', 'DBC Datei wird hochgeladen...', false);
      formData = new FormData();
      formData.append('dbc', fileInput.files[0]);
      request = new XMLHttpRequest();
      request.open('POST', '/api/dbc', true);
      request.onreadystatechange = function() {
        var result;
        if (request.readyState !== 4) {
          return;
        }

        result = parseJsonResponse(request);
        if (request.status >= 200 && request.status < 300) {
          fileInput.value = '';
          state.canDetailsById = {};
          setStatus('dbcStatus', result.message || 'DBC Datei gespeichert.', false);
          refreshAll(function() {
          }, function(error) {
            setStatus('dbcStatus', 'DBC Datei gespeichert. Status konnte nicht aktualisiert werden.', true);
          });
          return;
        }

        setStatus('dbcStatus', result.error || 'DBC Upload fehlgeschlagen.', true);
      };
      request.onerror = function() {
        setStatus('dbcStatus', 'Netzwerkfehler beim DBC Upload.', true);
      };
      request.send(formData);
      return false;
    }

    function toggleCanDetails(canId) {
      var key = getCanKey(canId);
      state.expandedCanIds[key] = !state.expandedCanIds[key];
      updateCanDetailRow(canId);
      if (state.expandedCanIds[key]) {
        refreshCanDetails(canId);
      }
    }

    function refreshLoop() {
      refreshStats(function() {
        refreshExpandedCanDetails();
      }, function(error) {
        setStatus('configStatus', error.message, true);
      });
    }

    document.getElementById('saveCanConfig').addEventListener('click', saveCanConfig);
    document.getElementById('dbcForm').addEventListener('submit', uploadDbc);

    state.snapshot = normalizeStateSnapshot({
      can: { supportedBaudRates: fallbackBaudRates, baudRate: fallbackBaudRates[0] }
    });
    render(state.snapshot);
    refreshAll(function() {}, function(error) {
      setStatus('configStatus', error.message, true);
    });
    state.refreshTimer = window.setInterval(refreshLoop, 1000);
  </script>
</body>
</html>
)HTML";

#endif