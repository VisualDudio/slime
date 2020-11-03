all:
	@$(MAKE) -C ./SlimeRouter/
	@$(MAKE) -C ./SlimeSocket/

debug:
	@$(MAKE) debug -C ./SlimeRouter/
	@$(MAKE) debug -C ./SlimeSocket/

release:
	@$(MAKE) release -C ./SlimeRouter/
	@$(MAKE) release -C ./SlimeSocket/

measure:
	@$(MAKE) measure -C ./SlimeRouter/
	@$(MAKE) measure -C ./SlimeSocket/

clean:
	@$(MAKE) -C ./SlimeRouter/ clean
	@$(MAKE) -C ./SlimeSocket/ clean
