package com.example.lowlatencymonitor.audio

object AudioEngine {
    init { System.loadLibrary("audioengine") }
    external fun start()
    external fun stop()

    external fun clearChain()
    external fun addNode(type: String): Int
    external fun setParam(nodeIndex: Int, paramId: Int, value: Float)
}
