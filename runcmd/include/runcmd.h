#ifndef RUNCMD_H
#define RUNCMD_H

int runcmd(const char *, int *, const int[]);

void (*runcmd_onexit)(void);

#endif // RUNCMD_H
