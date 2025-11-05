package com.example.lowlatencymonitor

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Bundle
import android.os.SystemClock
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import com.example.lowlatencymonitor.audio.AudioEngine

class MainActivity : ComponentActivity() {

    private val requestMicPermission = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { /* no-op */ }

    private var lastClickMs = 0L
    private fun debounced(ms: Long = 300): Boolean {
        val now = SystemClock.elapsedRealtime()
        if (now - lastClickMs < ms) return false
        lastClickMs = now
        return true
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val tvStatus = findViewById<TextView>(R.id.tvStatus)
        val btnToggle = findViewById<Button>(R.id.btnToggle)
        val btnAddDelay = findViewById<Button>(R.id.btnAddDelay)
        val seekGain = findViewById<SeekBar>(R.id.seekGain)
        val tvGainValue = findViewById<TextView>(R.id.tvGainValue)

        fun hasMicPermission(): Boolean =
            ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED

        fun isWired(): Boolean {
            val am = getSystemService(AudioManager::class.java)
            return am.getDevices(AudioManager.GET_DEVICES_OUTPUTS).any {
                it.type == AudioDeviceInfo.TYPE_WIRED_HEADPHONES || it.type == AudioDeviceInfo.TYPE_WIRED_HEADSET
            }
        }

        fun updateStatus() {
            tvStatus.text = if (isWired()) "Słuchawki przewodowe: OK" else "Podłącz słuchawki przewodowe"
        }
        updateStatus()

        var monitoring = false
        btnToggle.setOnClickListener {
            if (!debounced()) return@setOnClickListener
            if (!hasMicPermission()) {
                requestMicPermission.launch(Manifest.permission.RECORD_AUDIO); return@setOnClickListener
            }
            if (!isWired()) { updateStatus(); return@setOnClickListener }

            if (!monitoring) {
                AudioEngine.start()
                monitoring = true
                btnToggle.text = "Stop"
            } else {
                AudioEngine.stop()
                monitoring = false
                btnToggle.text = "Start"
            }
        }

        seekGain.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val g = progress / 100f
                tvGainValue.text = String.format("%.2f", g)
                AudioEngine.setParam(0, 0, g)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        btnAddDelay.setOnClickListener {
            if (!debounced()) return@setOnClickListener
            val idx = AudioEngine.addNode("delay")
            if (idx >= 0) {
                AudioEngine.setParam(idx, 0, 320f) // time ms
                AudioEngine.setParam(idx, 1, 0.35f) // feedback
                AudioEngine.setParam(idx, 2, 0.25f) // mix
            }
        }
    }

    override fun onStop() {
        super.onStop()
        AudioEngine.stop()
    }
}
