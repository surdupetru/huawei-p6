#!/usr/bin/perl -w

use Spreadsheet::ParseExcel;
use strict;

my $parser = new Spreadsheet::ParseExcel;
my $Book = $parser->Parse($ARGV[0]) ||
	die("Failed to open file $ARGV[0]\n");

if ( !defined $Book ) {
    die $parser->error(), ".\n";
}

open(OUTPUT, ">$ARGV[1]") ||
	die("Failed to open file $ARGV[1]\n");

($_ = $0) =~ s:.*/::;

print OUTPUT "\n/* DO NOT EDIT - Generated automatically by ".$_." */\n\n\n";
print OUTPUT "#include <linux/types.h>\n\n";

#Common
#------------------------------------------------------------------------------
my $Sheet = $Book->{Worksheet}[0];
print OUTPUT "const static u8 common[] = {\n";
foreach my $col ( 0 .. 12) {
	printf OUTPUT "\t0x%02X,0x%02X,0x%02X,0x%02X, /* %s|%s|%s|%s */\n",
		$Sheet->{Cells}[$col][27]->Value,
		$Sheet->{Cells}[$col][29]->Value,
		$Sheet->{Cells}[$col][31]->Value,
		$Sheet->{Cells}[$col][33]->Value,
		$Sheet->{Cells}[$col][26]->Value,
		$Sheet->{Cells}[$col][28]->Value,
		$Sheet->{Cells}[$col][30]->Value,
		$Sheet->{Cells}[$col][32]->Value;
}
# Timer period
foreach my $col ( 13 .. 14) {
	my $v = $Sheet->{Cells}[$col][28]->Value;
	printf OUTPUT "\t0x%s,0x%s,0x%s,0x%s, /* %s [%dms]*/\n",
		substr($v, 0, 2),
		substr($v, 2, 2),
		substr($v, 4, 2),
		substr($v, 6, 2),
		$Sheet->{Cells}[$col][26]->Value,
		$Sheet->{Cells}[$col][1]->Value;
}
# AVS para
foreach my $col ( 15 .. 28) {
	my $v = $Sheet->{Cells}[$col][28]->Value;
	printf OUTPUT "\t0x%s,0x%s,0x%s,0x%s, /* %s */\n",
		substr($v, 0, 2),
		substr($v, 2, 2),
		substr($v, 4, 2),
		substr($v, 6, 2),
		$Sheet->{Cells}[$col][26]->Value;
}
print OUTPUT"};\n";
#------------------------------------------------------------------------------


#CPU
#------------------------------------------------------------------------------
$Sheet = $Book->{Worksheet}[1];
print OUTPUT "\n\nconst static u8 cpu_profile[16][256] = {\n";
foreach my $col ( 0 .. 15) {
	if($Sheet->{Cells}[$col+1][2]->Value eq '') {
		last;
	}
	printf OUTPUT "\t{/*[%02d: %sMHz]*/\n\t\t0x%02X,0x%02X,0x00,0x%02X, /* Freq: %dMHz\@%.3fV hpm: %dMHz*/\n",
		($Sheet->{Cells}[$col+1][0]->Value),
		($Sheet->{Cells}[$col+1][1]->Value),
		($Sheet->{Cells}[$col+1][1]->Value)/256,
		($Sheet->{Cells}[$col+1][1]->Value)%256,
		($Sheet->{Cells}[$col+1][27]->Value),
		($Sheet->{Cells}[$col+1][2]->Value),
		($Sheet->{Cells}[$col+1][3]->Value),
		($Sheet->{Cells}[$col+1][2]->Value)/(($Sheet->{Cells}[$col+1][27]->Value)+1);
	foreach my $row ( 0 .. 4) {
		my $v = $Sheet->{Cells}[$col+1][29+$row*2]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* %s */\n",
			substr($v, 0, 2),
			substr($v, 2, 2),
			substr($v, 4, 2),
			substr($v, 6, 2),
			$Sheet->{Cells}[$col+1][28+$row*2]->Value;
	}
	print OUTPUT "\t\t0x00,0x00,0x00,0x00,\n\t\t0x00,0x00,0x00,0x00, ";
	foreach my $row ( 0 .. 13) {
		my $v1 = $Sheet->{Cells}[21+$row*20+$col][29]->Value;
		my $v2 = $Sheet->{Cells}[21+$row*20+$col][31]->Value;
		printf OUTPUT "/* [%02d-%s] */\n\t\t0x%s,0x%s,0x%s,0x%s, /* %s %sms¡ü %sMHz */\n",
			$row,
			$Sheet->{Cells}[19+$row*20][1]->Value,
			substr($v1, 0, 2),
			substr($v1, 2, 2),
			substr($v1, 4, 2),
			substr($v1, 6, 2),
			$Sheet->{Cells}[21+$row*20+$col][2]->Value,
			$Sheet->{Cells}[21+$row*20+$col][4]->Value,
			$Sheet->{Cells}[21+$row*20+$col][3]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* %s %sms¡ý %sMHz */\n",
			substr($v2, 0, 2),
			substr($v2, 2, 2),
			substr($v2, 4, 2),
			substr($v2, 6, 2),
			$Sheet->{Cells}[21+$row*20+$col][5]->Value,
			$Sheet->{Cells}[21+$row*20+$col][7]->Value,
			$Sheet->{Cells}[21+$row*20+$col][6]->Value;
		print OUTPUT "\t\t0x00,0x00,0x00,0x00,\n\t\t0x00,0x00,0x00,0x00, ";
	}

	print OUTPUT "\t},\n";
}
print OUTPUT"};\n";
#------------------------------------------------------------------------------


#GPU
#------------------------------------------------------------------------------
$Sheet = $Book->{Worksheet}[2];
print OUTPUT "\n\nconst static u8 gpu_profile[8][256] = {\n";
foreach my $col ( 0 .. 7) {
	if($Sheet->{Cells}[$col+1][2]->Value eq '') {
		last;
	}
	printf OUTPUT "\t{/*[%02d: %sMHz]*/\n\t\t0x%02X,0x%02X,0x%02X,0x%02X, /* [Core/Sharder/AXI]%.1f/%.1f/%.1fMHz\@%.3fV [C/S HPM]%d/%dMHz */\n",
		($Sheet->{Cells}[$col+1][0]->Value),
		($Sheet->{Cells}[$col+1][1]->Value),
		($Sheet->{Cells}[$col+1][1]->Value)/256,
		($Sheet->{Cells}[$col+1][1]->Value)%256,
		($Sheet->{Cells}[$col+1][11]->Value)-1,
		($Sheet->{Cells}[$col+1][18]->Value)-1,
		($Sheet->{Cells}[$col+1][2]->Value),
		($Sheet->{Cells}[$col+1][3]->Value),
		($Sheet->{Cells}[$col+1][4]->Value),
		($Sheet->{Cells}[$col+1][5]->Value),
		($Sheet->{Cells}[$col+1][2]->Value)/(($Sheet->{Cells}[$col+1][11]->Value)),
		($Sheet->{Cells}[$col+1][3]->Value)/(($Sheet->{Cells}[$col+1][18]->Value));
	foreach my $row ( 0 .. 3) {
		my $v = $Sheet->{Cells}[$col+1][27+$row*2]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* %s */\n",
			substr($v, 0, 2),
			substr($v, 2, 2),
			substr($v, 4, 2),
			substr($v, 6, 2),
			$Sheet->{Cells}[$col+1][26+$row*2]->Value;
	}
	print OUTPUT "\t\t0x00,0x00,0x00,0x00,\n\t\t0x00,0x00,0x00,0x00,\n\t\t0x00,0x00,0x00,0x00, ";
	foreach my $row ( 0 .. 13) {
		my $v1 = $Sheet->{Cells}[16+$row*15+$col][27]->Value;
		my $v2 = $Sheet->{Cells}[16+$row*15+$col][29]->Value;
		printf OUTPUT "/* [%02d-%s] */\n\t\t0x%s,0x%s,0x%s,0x%s, /* %s %sms¡ü %sMHz */\n",
			$row,
			$Sheet->{Cells}[14+$row*15][1]->Value,
			substr($v1, 0, 2),
			substr($v1, 2, 2),
			substr($v1, 4, 2),
			substr($v1, 6, 2),
			$Sheet->{Cells}[16+$row*15+$col][2]->Value,
			$Sheet->{Cells}[16+$row*15+$col][4]->Value,
			$Sheet->{Cells}[16+$row*15+$col][3]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* %s %sms¡ý %sMHz */\n",
			substr($v2, 0, 2),
			substr($v2, 2, 2),
			substr($v2, 4, 2),
			substr($v2, 6, 2),
			$Sheet->{Cells}[16+$row*15+$col][5]->Value,
			$Sheet->{Cells}[16+$row*15+$col][7]->Value,
			$Sheet->{Cells}[16+$row*15+$col][6]->Value;
		print OUTPUT "\t\t0x00,0x00,0x00,0x00,\n\t\t0x00,0x00,0x00,0x00, ";
	}

	print OUTPUT "\t},\n";
}
print OUTPUT"};\n";

#DDR
#------------------------------------------------------------------------------
$Sheet = $Book->{Worksheet}[3];
print OUTPUT "\n\nconst static u8 ddr_profile[8][256] = {\n";
foreach my $col ( 0 .. 7) {
	if($Sheet->{Cells}[$col+1][2]->Value eq '') {
		last;
	}
	printf OUTPUT "\t{/*[%02d: %sMHz]*/\n\t\t0x%02X,0x%02X,0x%s,0x%s, /* [Freq]%.1fMHz %s */\n",
		($Sheet->{Cells}[$col+1][0]->Value),
		($Sheet->{Cells}[$col+1][1]->Value),
		($Sheet->{Cells}[$col+1][1]->Value)/256,
		($Sheet->{Cells}[$col+1][1]->Value)%256,
		($Sheet->{Cells}[$col+1][27]->Value),
		($Sheet->{Cells}[$col+1][29]->Value),
		($Sheet->{Cells}[$col+1][2]->Value),
		($Sheet->{Cells}[$col+1][26]->Value);
	foreach my $row ( 0 .. 6) {
		my $v = $Sheet->{Cells}[$col+1][31+$row*2]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* %s */\n",
			substr($v, 0, 2),
			substr($v, 2, 2),
			substr($v, 4, 2),
			substr($v, 6, 2),
			$Sheet->{Cells}[$col+1][30+$row*2]->Value;
	}
	foreach my $row ( 0 .. 13) {
		my $v1 = $Sheet->{Cells}[21+$row*15+$col][27]->Value;
		my $v2 = $Sheet->{Cells}[21+$row*15+$col][29]->Value;
		my $v3 = $Sheet->{Cells}[21+$row*15+$col][31]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* [%02d-%s] */\n",
			substr($v1, 0, 2),
			substr($v1, 2, 2),
			substr($v1, 4, 2),
			substr($v1, 6, 2),
			$row,
			$Sheet->{Cells}[19+$row*15][1]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* Data[%s] || Cmd[%s] %sms¡ü %sMHz */\n",
			substr($v2, 0, 2),
			substr($v2, 2, 2),
			substr($v2, 4, 2),
			substr($v2, 6, 2),
			$Sheet->{Cells}[21+$row*15+$col][2]->Value,
			$Sheet->{Cells}[21+$row*15+$col][3]->Value,
			$Sheet->{Cells}[21+$row*15+$col][5]->Value,
			$Sheet->{Cells}[21+$row*15+$col][4]->Value;
		printf OUTPUT "\t\t0x%s,0x%s,0x%s,0x%s, /* Data[%s] && Cmd[%s] %sms¡ý %sMHz */\n",
			substr($v3, 0, 2),
			substr($v3, 2, 2),
			substr($v3, 4, 2),
			substr($v3, 6, 2),
			$Sheet->{Cells}[21+$row*15+$col][6]->Value,
			$Sheet->{Cells}[21+$row*15+$col][7]->Value,
			$Sheet->{Cells}[21+$row*15+$col][9]->Value,
			$Sheet->{Cells}[21+$row*15+$col][8]->Value;
		print OUTPUT "\t\t0x00,0x00,0x00,0x00,\n";

	}

	print OUTPUT "\t},\n";
}
print OUTPUT"};\n";
#------------------------------------------------------------------------------

close OUTPUT;
