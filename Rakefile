#
# Make it easy to package lockrun
require 'fileutils'

SEMVER='1.2'
iteration=`git rev-list HEAD --count`.chomp

task :default => [:usage]
task :help => [:usage]

task :usage do
  puts "rake deb:      Create an deb for lockrun"
  puts "rake lockrun:  Compile lockrun"
  puts
  puts "You must gem install fpm to build deb files"
end

task :deb => [:fakeroot, :lockrun] do
  sh %{ fpm -s dir -t deb -n lockrun \
    -v #{SEMVER} --iteration #{iteration} \
    --url https://github.com/pushcx/lockrun \
    --description "Lockrun allows you to use a lock to prevent multiple simultaneous executions of an executable." \
    -C .fakeroot -d libc6 --license "Public Domain" usr }
end

task :fakeroot => [:lockrun] do
  puts "iteration: #{iteration}"
  sh %{ rm -fr .fakeroot }
  FileUtils::mkdir_p '.fakeroot/usr/local/bin'
  sh %{ cp lockrun .fakeroot/usr/local/bin}
  sh %{ chown -R root.root .fakeroot }
end

task :lockrun do
  sh %{ gcc lockrun.c -o lockrun }
end

desc "Cleanup after build"
task :cleanup do
  sh %{ find . -name '*.un~' '*.o' -exec rm '{}' ';' }
  sh %{ rm -fr .fakeroot }
end
