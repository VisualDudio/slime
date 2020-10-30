all:
	@$(MAKE) debug -C ./SlimeRouter/
	@$(MAKE) -C ./SlimeSocket/

clean:
	@$(MAKE) -C ./SlimeRouter/ clean
	@$(MAKE) -C ./SlimeSocket/ clean
