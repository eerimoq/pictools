.PHONY: programmer

test-target:
	python3 -u tests/target.py

release-to-pypi:
	python setup.py sdist
	python setup.py bdist_wheel --universal
	twine upload dist/*

programmer:
	cd ramapp && $(MAKE)
	cd programmer && $(MAKE) generate_ramapp_upload_instructions_i
	cd programmer && $(MAKE) all

programmer-clean:
	cd ramapp && $(MAKE) clean
	cd programmer && $(MAKE) clean

programmer-dist:
	cd programmer && $(MAKE) dist
