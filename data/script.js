// Configuration
let config = {
    strips: [],
    mode: 0,
    brightness: 100,
    manualMode: false
};

// DOM Elements
const elements = {
    stripsConfig: document.getElementById('stripsConfig'),
    totalLeds: document.getElementById('totalLeds'),
    activeStrips: document.getElementById('activeStrips'),
    brightnessValue: document.getElementById('brightnessValue'),
    freeHeap: document.getElementById('freeHeap'),
    connectionStatus: document.getElementById('connectionStatus'),
    brightnessSlider: document.getElementById('brightnessSlider'),
    brightnessPercent: document.getElementById('brightnessPercent'),
    colorPicker: document.getElementById('colorPicker'),
    colorR: document.getElementById('colorR'),
    colorG: document.getElementById('colorG'),
    colorB: document.getElementById('colorB'),
    manualControl: document.getElementById('manualControl')
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    loadConfig();
    updateStatus();
    setInterval(updateStatus, 5000);
});

// Load configuration
async function loadConfig() {
    try {
        const response = await fetch('/api/config');
        if (!response.ok) throw new Error('Network error');
        
        config = await response.json();
        updateUI();
        updateConnectionStatus(true);
    } catch (error) {
        console.error('Failed to load config:', error);
        updateConnectionStatus(false);
        showToast('⚠️ Connection lost. Retrying...', 'warning');
        setTimeout(loadConfig, 3000);
    }
}

// Update UI
function updateUI() {
    updateStripsConfig();
    updateModeButtons();
    updateBrightnessDisplay();
    updateStats();
    
    // Show/hide manual control
    elements.manualControl.style.display = config.manualMode ? 'block' : 'none';
}

// Update strips configuration
function updateStripsConfig() {
    let html = '';
    config.strips.forEach((strip, index) => {
        html += `
            <div class="strip-card ${strip.enabled ? 'active' : ''}">
                <div class="strip-header">
                    <h4>GPIO ${strip.pin}</h4>
                    <label class="toggle">
                        <input type="checkbox" ${strip.enabled ? 'checked' : ''} 
                               onchange="toggleStrip(${index}, this.checked)">
                        <span class="toggle-slider"></span>
                    </label>
                </div>
                <input type="number" class="led-count" 
                       value="${strip.ledCount}" min="0" max="300"
                       placeholder="LED Count"
                       onchange="updateStripCount(${index}, this.value)">
            </div>
        `;
    });
    elements.stripsConfig.innerHTML = html;
}

// Update mode buttons
function updateModeButtons() {
    document.querySelectorAll('.mode-btn').forEach(btn => {
        const mode = parseInt(btn.dataset.mode);
        btn.classList.toggle('active', mode === config.mode);
    });
}

// Update brightness display
function updateBrightnessDisplay() {
    const percent = Math.round((config.brightness / 255) * 100);
    elements.brightnessSlider.value = config.brightness;
    elements.brightnessPercent.textContent = `${percent}%`;
    elements.brightnessValue.textContent = `${percent}%`;
}

// Update statistics
function updateStats() {
    const totalLeds = config.strips.reduce((sum, strip) => 
        strip.enabled ? sum + strip.ledCount : sum, 0);
    const activeStrips = config.strips.filter(s => s.enabled).length;
    
    elements.totalLeds.textContent = totalLeds;
    elements.activeStrips.textContent = activeStrips;
}

// Update system status
async function updateStatus() {
    try {
        const response = await fetch('/api/status');
        if (!response.ok) throw new Error('Status error');
        
        const status = await response.json();
        elements.freeHeap.textContent = `${Math.round(status.freeHeap / 1024)} KB`;
        updateConnectionStatus(true);
    } catch (error) {
        updateConnectionStatus(false);
    }
}

// Connection status indicator
function updateConnectionStatus(connected) {
    const statusEl = elements.connectionStatus;
    if (connected) {
        statusEl.innerHTML = '<i class="bi bi-wifi"></i> Connected';
        statusEl.style.background = 'rgba(34, 197, 94, 0.1)';
        statusEl.style.borderColor = '#22c55e';
        statusEl.style.color = '#22c55e';
    } else {
        statusEl.innerHTML = '<i class="bi bi-wifi-off"></i> Disconnected';
        statusEl.style.background = 'rgba(239, 68, 68, 0.1)';
        statusEl.style.borderColor = '#ef4444';
        statusEl.style.color = '#ef4444';
    }
}

// API Calls
async function saveConfig() {
    const newConfig = {
        strips: []
    };
    
    document.querySelectorAll('.strip-card').forEach((card, index) => {
        const enabled = card.querySelector('input[type="checkbox"]').checked;
        const ledCount = parseInt(card.querySelector('.led-count').value) || 0;
        
        newConfig.strips.push({
            pin: config.strips[index].pin,
            ledCount: ledCount,
            enabled: enabled
        });
    });
    
    try {
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(newConfig)
        });
        
        if (response.ok) {
            showToast('✅ Configuration saved!', 'success');
            await loadConfig();
        }
    } catch (error) {
        showToast('❌ Save failed!', 'error');
    }
}

async function setMode(mode) {
    try {
        await fetch('/api/mode', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({mode: mode})
        });
        
        config.mode = mode;
        config.manualMode = (mode === 255);
        updateUI();
    } catch (error) {
        showToast('❌ Mode change failed', 'error');
    }
}

function updateBrightness(value) {
    const percent = Math.round((value / 255) * 100);
    elements.brightnessPercent.textContent = `${percent}%`;
    
    // Debounce
    clearTimeout(this.brightnessTimeout);
    this.brightnessTimeout = setTimeout(() => setBrightness(value), 200);
}

async function setBrightness(value) {
    try {
        await fetch('/api/brightness', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({brightness: parseInt(value)})
        });
        config.brightness = parseInt(value);
        updateBrightnessDisplay();
    } catch (error) {
        showToast('❌ Brightness update failed', 'error');
    }
}

function setColorFromPicker() {
    const hex = elements.colorPicker.value;
    const r = parseInt(hex.substr(1, 2), 16);
    const g = parseInt(hex.substr(3, 2), 16);
    const b = parseInt(hex.substr(5, 2), 16);
    
    elements.colorR.value = r;
    elements.colorG.value = g;
    elements.colorB.value = b;
    
    setColor(r, g, b);
}

function setColorFromRGB() {
    const r = parseInt(elements.colorR.value) || 0;
    const g = parseInt(elements.colorG.value) || 0;
    const b = parseInt(elements.colorB.value) || 0;
    
    const hex = '#' + 
        r.toString(16).padStart(2, '0') +
        g.toString(16).padStart(2, '0') +
        b.toString(16).padStart(2, '0');
    
    elements.colorPicker.value = hex;
    setColor(r, g, b);
}

async function setColor(r, g, b) {
    try {
        await fetch('/api/color', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({r, g, b})
        });
        showToast('🎨 Color updated', 'success');
    } catch (error) {
        showToast('❌ Color update failed', 'error');
    }
}

async function setSingleLED() {
    if (!config.manualMode) {
        showToast('⚠️ Enable Manual Mode first', 'warning');
        return;
    }
    
    const strip = parseInt(document.getElementById('stripIndex').value) || 0;
    const led = parseInt(document.getElementById('ledIndex').value) || 0;
    const hex = document.getElementById('ledColor').value;
    
    const r = parseInt(hex.substr(1, 2), 16);
    const g = parseInt(hex.substr(3, 2), 16);
    const b = parseInt(hex.substr(5, 2), 16);
    
    try {
        await fetch('/api/led', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({strip, led, r, g, b})
        });
        showToast(`💡 LED ${led} on Strip ${strip} updated`, 'success');
    } catch (error) {
        showToast('❌ LED update failed', 'error');
    }
}

// Helper functions
function toggleStrip(index, enabled) {
    const card = elements.stripsConfig.children[index];
    card.classList.toggle('active', enabled);
}

function updateStripCount(index, count) {
    config.strips[index].ledCount = parseInt(count) || 0;
    updateStats();
}

// Toast notification
function showToast(message, type = 'info') {
    // Remove existing toast
    const existing = document.querySelector('.toast');
    if (existing) existing.remove();
    
    // Create toast
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `
        <i class="bi ${getToastIcon(type)}"></i>
        <span>${message}</span>
    `;
    
    // Style
    toast.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${getToastColor(type)};
        color: white;
        padding: 15px 25px;
        border-radius: 12px;
        display: flex;
        align-items: center;
        gap: 12px;
        z-index: 1000;
        animation: slideIn 0.3s ease;
        box-shadow: 0 10px 25px rgba(0,0,0,0.2);
    `;
    
    document.body.appendChild(toast);
    
    // Auto remove
    setTimeout(() => {
        toast.style.animation = 'slideOut 0.3s ease forwards';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
    
    // Add animations
    const style = document.createElement('style');
    style.textContent = `
        @keyframes slideIn {
            from { transform: translateX(100%); opacity: 0; }
            to { transform: translateX(0); opacity: 1; }
        }
        @keyframes slideOut {
            from { transform: translateX(0); opacity: 1; }
            to { transform: translateX(100%); opacity: 0; }
        }
    `;
    document.head.appendChild(style);
}

function getToastIcon(type) {
    const icons = {
        success: 'bi-check-circle-fill',
        error: 'bi-x-circle-fill',
        warning: 'bi-exclamation-triangle-fill',
        info: 'bi-info-circle-fill'
    };
    return icons[type] || 'bi-info-circle-fill';
}

function getToastColor(type) {
    const colors = {
        success: 'linear-gradient(135deg, #22c55e, #16a34a)',
        error: 'linear-gradient(135deg, #ef4444, #dc2626)',
        warning: 'linear-gradient(135deg, #f59e0b, #d97706)',
        info: 'linear-gradient(135deg, #3b82f6, #2563eb)'
    };
    return colors[type] || colors.info;
}