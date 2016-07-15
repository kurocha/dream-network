
#
#  This file is part of the "Teapot" project, and is released under the MIT license.
#

teapot_version "1.0"

define_target "dream-network" do |target|
	target.build do |environment|
		source_root = target.package.path + 'source'
		
		copy headers: source_root.glob('Dream/**/*.hpp')
		
		build static_library: "DreamNetwork", source_files: source_root.glob('Dream/**/*.cpp')
	end
	
	target.depends :platform
	target.depends "Language/C++11"
	
	target.depends "Build/Files"
	target.depends "Build/Clang"

	target.depends "Library/DreamEvents"
	target.depends "Library/Dream"
	
	target.provides "Library/DreamNetwork" do
		append linkflags {install_prefix + "lib/libDreamNetwork.a"}
	end
end

define_target "dream-network-tests" do |target|
	target.build do |environment|
		test_root = target.package.path + 'test'
		
		run tests: "DreamNetwork", source_files: test_root.glob('Dream/**/*.cpp')
	end
	
	target.depends "Library/UnitTest"
	target.depends "Library/DreamNetwork"

	target.provides "Test/DreamNetwork"
end

define_configuration "test" do |configuration|
	configuration[:source] = "https://github.com/kurocha"
	
	configuration.require "platforms"
	configuration.require "build-files"
	
	configuration.require "dream"
	configuration.require "dream-events"
	
	configuration.require "euclid"
	configuration.require "unit-test"
	
	configuration.require "language-cpp-class"
end
