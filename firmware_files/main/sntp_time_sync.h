#ifndef MAIN_SNTP_TIME_SYNC_H
#define MAIN_SNTP_TIME_SYNC_H

/**
 * Starts the SNTP server synchronisation task
 */
void sntp_time_sync_task_start(void);

/**
 * Returns local time if set
 * @returns local time buffer
 */
char* sntp_time_sync_get_time(void);

/**
 * Ensure time is synchronised first
 * @return bool
 */
bool sntp_time_synced();

#endif /* MAIN_SNTP_TIME_SYNC_H */