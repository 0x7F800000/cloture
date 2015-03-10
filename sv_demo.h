#pragma once

void SV_StartDemoRecording(client_t *client, const char *filename, int forcetrack);
void SV_WriteDemoMessage(client_t *client, sizebuf_t *sendbuffer, bool clienttoserver);
void SV_StopDemoRecording(client_t *client);
void SV_WriteNetnameIntoDemo(client_t *client);
