#pragma once

// Internal boundary for the controller translation pipeline that runs between
// raw input polling and output transport send.

void processPolledInputFrame(bool updated);
void cacheProcessedControllerOutputState();
void finalizeControllerFrameForOutput();
void restoreControllerFrameAfterOutputSend();
