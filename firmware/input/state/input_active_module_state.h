#pragma once

class RZInputModule;

RZInputModule* activeInputModule();

void setActiveInputModule(RZInputModule* module);

bool hasActiveInputModule();
