@echo off
echo Syncing fork with master repo...
git fetch upstream
git checkout master
git merge upstream/master

pause