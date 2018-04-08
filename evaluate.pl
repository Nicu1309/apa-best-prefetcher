#!/usr/bin/perl
my @configs = ("small_llc", "low_bandwidth", "scramble_loads");
my $pref_folder = "example_prefetchers";
my $trace_folder = "traces";
my $target_folder = "bin";
my $base_target = "dpc2sim";
my $base_model = "lib/dpc2sim.a";
my @compiled_names;
my %retired_stats;
my %cycles_stats;
my %ipc_stats;

#Create target directory if does not exist
sub check_target_dir {
	if (not -d $target_folder){
		print "========================================================\n";
		print "Creating folder to place compiled models in\n";
		print "========================================================\n";
		`mkdir bin`; 
	}
}

sub compile_pref_folder {
	my @pref_to_test = `ls $pref_folder`;
	foreach my $pref_full_name (@pref_to_test) {
		next if ($pref_full_name =~ m/skeleton/);
		$pref_full_name =~ s/\n//;
		my $target = "${base_target}_$pref_full_name";
		$target =~ s/\.c//;
		`gcc -Wall -o $target_folder/$target $pref_folder/$pref_full_name $base_model\n`;
		push(@compiled_names,$target);
	}
}

sub compile_prefetcher {
	my ($pref_folder,$pref_full_name) = @_;
	$pref_full_name =~ s/\n//;
	my $target = "${base_target}_$pref_full_name";
	$target =~ s/\.c//;
	`gcc -Wall -o $target_folder/$target $pref_folder/$pref_full_name $base_model\n`;
	push(@compiled_names,$target);
}

sub execute_all_traces {
	my ($target) = @_;
	my @traces_to_test = `ls $trace_folder`;
	foreach my $trace_full_name (@traces_to_test) {
		$trace_full_name =~ s/\n//;
		my ($trace_id, $trace_ext) = ($trace_full_name =~ m/(\w+)\.(.+)/);
		my $cat = "cat";
		if ($trace_ext =~ m/.*gz*/i){
			$cat = "zcat";
		}
		foreach my $conf (@configs){
			my $command = "$cat $trace_folder/$trace_full_name | ./$target_folder/$target -$conf -hide_heartbeat";
			print "\n===============================\nExecuting...\n============================\n";
			print "$command\n";
			my $result = `$command`;
			my ($retired, $cycles, $ipc) = ($result =~ m/retired:\s+(\w+)\s+Cycles\selapsed:\s+(\w+)\s+IPC:\s+(.+)/i);
			$result =~ m/Simulation complete\.(.+)/;
			print "$result\n";
			print "Retired: $retired\n";
			print "Cycles: $cycles\n";
			print "IPC: $ipc\n";
			break;
		}
		break;
	}
}
	
#For each prefetcher in pref_folder, for each trace, for each configuration
#Execute and collect results
#{{{
check_target_dir();
compile_pref_folder();
foreach my $target (@compiled_names){
	execute_all_traces($target);
}
#}}}
