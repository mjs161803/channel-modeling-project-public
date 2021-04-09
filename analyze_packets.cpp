/***********************************
 * This program reads in two CSV files containing
 * data on LoRa packets transmitted by a mobile
 * device and received by a gateway device.
 *
 * The program then performs several analyses on the 
 * data:
 * 	1) Calculates value of gamma for a Simplified
 * 	Path Loss Model
 * 	2) Calculates the standard deviation of 
 * 	Shadow Fading (sigma_SF)
 * 	3) Calculates and plots a figure showing a
 * 	scatterplot of path loss vs distance.  A
 * 	plot of the derived path loss model is 
 * 	overlaid on top of the scatterplot.
 * 	4) Calculates and prints a table showing 
 * 	packet loss % per binned distance
 * 	5) Generates a plot showing % packet loss vs
 * 	binned distances
 *
 ******************************************/

#ifndef INCL_IOSTREAM
#define INCL_IOSTREAM
#include <iostream>
#endif

#ifndef INCL_FSTREAM
#define INCL_FSTREAM
#include <fstream>
#endif

#ifndef INCL_STRING
#define INCL_STRING
#include <string>
#endif

#ifndef INCL_VECTOR
#define INCL_VECTOR
#include <vector>
#endif

// Data structure for results_array
struct result_elem {
	long packet_id;
	bool successful;
	double tx_lat;
	double tx_long;
	double tx_power;

	double rx_lat;
	double rx_long;

	double distance_km;
	double rssi_dbm;
	double snr_db;
	double pathloss_measured_db;
	double pathloss_modeled_db;
};

// Function to take in row of text, and return a vector containing
// the fields of the CSV row
enum class CSVState {
	UnquotedField,
	QuotedField,
	QuotedQuote
};

std::vector<std::string> readCSVRow(const std::string &row) {
	CSVState state = CSVState::UnquotedField;
	std::vector<std::string> fields {""};
	size_t i = 0; // index of current field
	for (char c : row) {
		switch (state) {
			case CSVState::UnquotedField:
				switch (c) {
					case ',': // end of field
						fields.push_back(""); i++;
						break;
					case '"': state = CSVState::QuotedField;
						break;
					default: fields[i].push_back(c);
						break;
				}
				break;
			case CSVState::QuotedField:
				switch (c) {
					case '"': state = CSVState::QuotedQuote;
						  break;
					default:  fields[i].push_back(c);
						  break;
				}
				break;
			case CSVState::QuotedQuote:
				switch (c) {
					case ',': // , after closing quote
						  fields.push_back(""); i++;
						  state = CSVState::UnquotedField;
						  break;
					case '"': // "" -> "
						  fields[i].push_back('"');
						  state = CSVState::QuotedField;
						  break;
					default: // end of quote
						  state = CSVState::UnquotedField;
						  break;
				}
				break;
		}
	}
	return fields;
}

// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>> readCSV(std::ifstream &in) {
	std::vector<std::vector<std::string>> table;
	std::string row;
	while (!in.eof()) {
		std::getline(in, row);
		if (in.bad() || in.fail()) {
			break;
		}
		auto fields = readCSVRow(row);
		table.push_back(fields);
	}
	return table;
}

std::vector<result_elem> packet_compare(std::vector<std::vector<std::string>> &tx_table, std::vector<std::vector<std::string>> &rx_table) {
	// iterate over each row in tx_table.  for each:
	//    - generate entry for results_table, populate...
	//       - packet_id
	//       - tx_lat
	//       - tx_long
	//       - all others to NULL
	//    - search rx_table for matching packet_id
	//    - if match found, populate... 
	//       - rx_lat
	//       - rx_long
	//       - distance_km
	//       - rssi
	//       - snr
	//       - pathloss_measured_db 
	std::vector<result_elem> results_table;
	result_elem curr_res;

	for (auto &tx_elem : tx_table) {
		curr_res.packet_id = std::stol(tx_elem.at(0));
		curr_res.tx_lat = std::stod(tx_elem.at(1));
		curr_res.tx_long = std::stod(tx_elem.at(2));
		curr_res.tx_power = std::stod(tx_elem.at(4));
		
		curr_res.successful = 0;
		curr_res.distance_km = 0;
		curr_res.rx_lat = 0;
		curr_res.rx_long = 0;
		curr_res.rssi_dbm = 0;
		curr_res.snr_db = 0;
		curr_res.pathloss_measured_db = 0;
		curr_res.pathloss_modeled_db = 0;

		results_table.push_back(curr_res);
	}
		
	return results_table;
}

int main() {
	// open up CSV files and read them to tables
	std::ifstream ifs_tx, ifs_rx;
	ifs_tx.open ("tx_packets - 6 APR 2021.csv", std::ifstream::in);
	ifs_rx.open ("rx_packets - 6 APR 2021.csv", std::ifstream::in);

	std::vector<std::vector<std::string>> tx_table = readCSV(ifs_tx);
	std::vector<std::vector<std::string>> rx_table = readCSV(ifs_rx);
	
	// Compare tx_table and rx_table to generate results_table
	std::vector<result_elem> results_table = packet_compare(tx_table, rx_table);
	
	// Analyze results_table and calculate gamma value
	//double gamma = calc_gamma(results_table);

	// Calculate shadow fading standard deviation
	//double sigma_sf = calc_sigma_sf(results_table);

	// Generate scatter plot of PL vs distance, along with overlay
	// of Simplified Path Loss model generated earlier
	//gen_scatterplot(results_table, gamma);

	// Generate table of packet loss % binned by distance
	//std::vector<double> loss_table = gen_loss_table(results_table);

	// Print out packet loss table
	//print_table(loss_table);

	// Generate plot of packet loss % vs binned distance
	//gen_lossplot(loss_table);

	ifs_tx.close();
	ifs_rx.close();

	return 0;
}
