# Transactional Memcached

This repository contains code for a transactional version of memcached, based
on the Ruan et al. paper from ASPLOS 2014.  In that work, we developed
several versions of memcached.  We expect that only the last version
(internally, version 12b) will be of interest to TM users, and thus to avoid
confusion it is, for now, the only version of code present in the repository.

This work was performed before we implemented a proper solution for condition
variables in legacy code.  We hope to provide an updated version soon.

If you find any bugs in this code, please contact Mike Spear
(spear@cse.lehigh.edu).

If you use this code in a publication, please be sure to cite Ruan et
al. ASPLOS 2014.
