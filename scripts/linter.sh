scripts/cpplint.py src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h 2> LINT
grep -v 'should almost always be at the end of the previous line' LINT  > LINT2
grep -v 'Lines should very rarely be longer than 100 characters'  LINT2 > LINT
rm LINT2
cat LINT
