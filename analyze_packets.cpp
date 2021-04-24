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

#ifndef INCL_ALGORITHM
#define INCL_ALGORITHM
#include <algorithm>
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

#ifndef INCL_CMATH
#define INCL_CMATH
#include <cmath>
#endif

#ifndef INCL_MATPLOTLIB
#define INCL_MATPLOTLIB
#include "matplotlibcpp.h"
#endif

namespace plt = matplotlibcpp;

// DATA STRUCTURES 

enum class CSVState {
	UnquotedField,
	QuotedField,
	QuotedQuote
};

struct result_elem {
	long packet_id;
	bool successful;
	double tx_lat;
	double tx_long;
	double tx_power;

	double rx_lat;
	double rx_long;

	double distance_m;
	double rssi_dbm;
	double snr_db;
	double pathloss_measured_db;
	double pathloss_modeled_db;
};

class ChannelModel {
	private:
		double k_param;
		double gamma_param;
		double d_ref;
		double std;
	public:
		ChannelModel();

		void set_k_param(const double new_k_param);
		void set_gamma_param(const double new_gamma_param);
		void set_d_ref(const double new_d_ref);

		double get_k_param();
		double get_gamma_param();
		double get_d_ref();
		double get_std();

		void optimize_model(std::vector<result_elem> &results_table);

};

// FUNCTION DECLARATIONS
double calc_distance (double, double, double, double);
double calc_gamma (std::vector<result_elem>&, double&, double&);
double calc_sigma_sf (std::vector<result_elem>&); 


// FUNCTION DEFINITIONS

// Function to take in row of text, and return a vector containing
// the fields of the CSV row

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
	//       - distance_m
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
		curr_res.distance_m = calc_distance(curr_res.tx_lat, curr_res.tx_long, 38.34741111, -78.22621111);
		curr_res.rx_lat = 0;
		curr_res.rx_long = 0;
		curr_res.rssi_dbm = 0;
		curr_res.snr_db = 0;
		curr_res.pathloss_measured_db = 0;
		curr_res.pathloss_modeled_db = 0;

		for (auto &rx_elem : rx_table) {
			double rx_id = std::stol(rx_elem.at(0));
			if (rx_id == curr_res.packet_id) { // packet was successfully received
				curr_res.successful = 1;
				curr_res.rx_lat = std::stod(rx_elem.at(1));
				curr_res.rx_long = std::stod(rx_elem.at(2));
				curr_res.rssi_dbm = std::stod(rx_elem.at(4));
				curr_res.snr_db = std::stod(rx_elem.at(5));
				if (curr_res.snr_db < 0) {
					curr_res.pathloss_measured_db = curr_res.rssi_dbm - curr_res.tx_power + curr_res.snr_db;
				}
				else {
					curr_res.pathloss_measured_db = curr_res.rssi_dbm - curr_res.tx_power;
				}
				curr_res.distance_m = ::calc_distance(curr_res.tx_lat, curr_res.tx_long, curr_res.rx_lat, curr_res.rx_long); 
			}
		}
		results_table.push_back(curr_res);
	}
		
	return results_table;
}

double calc_distance(double lat1, double lon1, double lat2, double lon2) {
	// use haversine formula to calculate distance between points
	// parameters lat1, lon1, lat2, lon2 are all in degrees
	// 
	// The calculated distance returned is in units of meters
	long double r = 6371e3;
	long double lat1_rad = lat1 * (3.141592653 / 180.0);
	long double lat2_rad = lat2 * (3.141592653 / 180.0);
	long double delta_lat = (lat2 - lat1) * (3.141592653/180.0);
	long double delta_lon = (lon2 - lon1) * (3.141592653/180.0);

	long double a = pow(sin(delta_lat/2.0), 2.0) + (cos(lat1_rad)*cos(lat2_rad)*pow(sin(delta_lon/2.0), 2.0));
	long double c = 2.0 * atan2( pow(a, 0.5), pow((1.0-a), 1.0/2.0));
	long double d = r * c;
	//std::cout << "lat1_rad: " << lat1_rad << ", lat2_rad: " << lat2_rad << ", delta_lat: " << delta_lat << ", delta_lon: " << delta_lon << ", a: " << a << ", c: " << c << ", d: " << d << std::endl;
	return (double)d;
}

double calc_gamma(std::vector<result_elem> &results_table, double &curr_dist, double &curr_pl) {
	// This function calculates a value for Gamma using the 
	// Minimum Mean Squared Error (MMSE).  The MSE is calculated
	// as a function of gamma "F(g)".  That function is then
	// differentiated with respect to gamma, set equal to zero
	// and then solved for gamma.
	//
	// The rough algorithm:
	//    1) pick value for d_o (for use in the Simplified Path Loss Model)
	//    2) calculate K parameter using FSPL at distance d_o
	//    3) iterate over results table, summing up terms for 
	//       the equation F(g)
	//
	//       MSE = Sum [over all measurement points] (PL_measured - PL_modeled)^2
	//
	//    4) differentiate F(g) by multiplying 'z' term by 2
	//    5) set F'(g) = 0 and solve for gamma
	
	double gamma, x {0.0}, y {0.0}, z{0.0};
	double wavelength = 299000000.0 / 915000000.0;
	double d_o = curr_dist; 		
	double k = curr_pl; 	
	
	for (auto &cur_res : results_table) {
		if (cur_res.successful == 1) {
			x += 2 * (cur_res.pathloss_measured_db - k) * (10*log10(cur_res.distance_m / d_o));
			y += pow((10*log10(cur_res.distance_m / d_o)), 2);
		}
	}

	z = 2.0 * y; // differentiate F(g)
	gamma = -x / z;

	return gamma;
}

double calc_sigma_sf(std::vector<result_elem> &results_table) {
	double sf_sigma {0.0}, error_sum {0.0};
	int n {0};

	for (auto &curr_res : results_table) {
		if (curr_res.successful == 1) {
			error_sum += pow((curr_res.pathloss_measured_db - curr_res.pathloss_modeled_db), 2);	
			++n;
		}
	}
	
	sf_sigma = pow( (error_sum / n), 0.5);

	return sf_sigma;
}

void gen_scatterplot(std::vector<result_elem>& results_table, ChannelModel &my_channel_model) {
	std::vector<double> x1, x2, y1, y2;
	double wavelength = 299000000.0 / 915000000.0;
	double d_o = my_channel_model.get_d_ref(); 	
	double k = my_channel_model.get_k_param(); 
	double gamma = my_channel_model.get_gamma_param();

	for (auto &curr_res : results_table) {
		if (curr_res.successful == 1) {
			x1.push_back(curr_res.distance_m);
			y1.push_back(curr_res.pathloss_measured_db);
		}
	}

	double max_dist {0.0}, min_dist {0.0};
	for (auto &elem : x1) {
		min_dist = (min_dist < elem)? min_dist : elem;
		max_dist = (max_dist > elem)? max_dist : elem;
	}

	double num_points = x1.size();
	double model_resolution = (max_dist - min_dist) / num_points;
	
	for (int i = 0; i < num_points; ++i) {
		x2.push_back((double)i * model_resolution);
		y2.push_back(k - (10 * gamma * log10(x2.at(i) / d_o)));
	}

	plt::scatter(x1, y1);
	plt::named_plot("Modeled Path Loss", x2,y2, "r-");
	plt::legend();
	plt::title("Measured and Modeled Path Loss");
	plt::show();
}

std::vector<double> gen_loss_table(std::vector<result_elem> &results_table) {
	double max_dist {0.0};
	double bin_size = 100.0; // meters
	for (auto &elem : results_table) {
		max_dist = (max_dist > elem.distance_m)? max_dist : elem.distance_m;
	}

	//std::cout << "======================================" << std::endl;
	//std::cout << "***    PACKET LOSS STATISTICS      ***" << std::endl;
	//std::cout << "======================================" << std::endl;

	//std::cout << std::endl << "Maximum distance: " << max_dist << std::endl;

	int num_bins = ceil(max_dist / bin_size);
	//std::cout << "Number of " << bin_size << "m distance bins: " << num_bins << std::endl << std::endl;

	std::vector<double> loss_table(num_bins, 0.0);
	std::vector<double> tx_table(num_bins, 0.0);
	std::vector<double> rx_table(num_bins, 0.0);

	for (auto &elem : results_table) {
		int bin_num = floor(elem.distance_m / bin_size);
		++tx_table.at(bin_num);
		if (elem.successful) ++rx_table.at(bin_num);
	}

	//std::cout << std::endl << "tx_table:" << std::endl;
	//for (auto &elem : tx_table) {
	//	std::cout << elem << std::endl;
	//}

	//std::cout << std::endl << "rx_table:" << std::endl;
	//for (auto &elem : rx_table) {
	//	std::cout << elem << std::endl;
	//}

	int bin_num = 0;
	for (auto &elem : loss_table) {
		//std::cout << (bin_num * bin_size) << " - " << (((bin_num+1)*bin_size)-1) << " meters: ";
		elem = rx_table.at(bin_num) / tx_table.at(bin_num);
		//std::cout << elem << std::endl;
		++bin_num;
	}	       
	
	return loss_table;
}

void gen_lossplot(std::vector<double> &loss_table) {
	std::vector<double> xaxis;
	std::vector<double> yaxis;
	double bin_size = 100.0;

	double i = 1.0;
	for (auto &elem : loss_table) {
		xaxis.push_back(i*bin_size);
		yaxis.push_back(1.0 - elem);
		++i;
	}

	plt::named_plot("Packet Loss %", xaxis, yaxis);
	plt::legend();
	plt::title("Packet Loss % vs Distance (In Meters)");
	plt::show();	
}

ChannelModel::ChannelModel() {
	k_param = -90.0;
	gamma_param = 2.0;
	d_ref = 1.0;
	std = 100.0;
}

void ChannelModel::set_k_param(const double new_k_val) {
	k_param = new_k_val;
}

void ChannelModel::set_gamma_param(const double new_gamma_val) {
	gamma_param = new_gamma_val;
}

void ChannelModel::set_d_ref(const double new_d_ref) {
	d_ref = new_d_ref;
}

double ChannelModel::get_d_ref() {
	return d_ref;
}

double ChannelModel::get_gamma_param() {
	return gamma_param;
}

double ChannelModel::get_k_param() {
	return k_param;
}

double ChannelModel::get_std() {
	return std;
}

void ChannelModel::optimize_model(std::vector<result_elem> &results_table) {
	// Brute Force method to determine what parameters minimize the 
	// standard deviation from the model
	//
	// iterate over d_ref
	// 	iterate over k (0 to -160) @ d_ref
	// 		- calculate gamma
	// 		- calculate std
	// 		- if current std < min_std, then use current d_ref, k, gamma and std

	double optimized_d_ref {0.0}, optimized_k_param {0.0}, optimized_gamma_param {0.0}, optimized_std {100.0};
	double max_dist {0.0}, min_dist {100.0};

	for (auto &elem : results_table) {
		min_dist = (min_dist < elem.distance_m)? min_dist : elem.distance_m;
		max_dist = (max_dist > elem.distance_m)? max_dist : elem.distance_m;
	}

	// iterate all d_ref's from min to max in 1m increments
	for(double curr_dist = min_dist; curr_dist < max_dist; curr_dist += 1.0) {

		// iterate over all pathlosses possible from 0 dB to -160 dB, in 1dB increments
		for (double curr_pl = 0.0; curr_pl >= -160.0; curr_pl += -1.0) {
			
			double curr_gamma = ::calc_gamma(results_table, curr_dist, curr_pl); 
			
			// calculate std of model using current parameters
			// first, populate results_table with modeled path loss values using this new gamma value
			for (auto &cur_res : results_table) {
				cur_res.pathloss_modeled_db = curr_pl - (10.0 * curr_gamma * log10(cur_res.distance_m / curr_dist));
			}
			// now calculate std
			double curr_std = ::calc_sigma_sf(results_table);

			// check if new std is lower than the current optimized_std, update
			// optimized_d_ref, optimized_k_param, optimized_gamma_param, and
			// optimized_std if it is.
			if (curr_std < optimized_std) {
				optimized_d_ref = curr_dist;
				optimized_k_param = curr_pl;
				optimized_gamma_param = curr_gamma;
				optimized_std = curr_std;
			}

		}
	}


	d_ref = optimized_d_ref;
	k_param = optimized_k_param;
	gamma_param = optimized_gamma_param;
	std = optimized_std;

}


// MAIN FUNCTION
int main() {
	ChannelModel my_channel_model;

	// open up CSV files and read them to tables
	std::ifstream ifs_tx, ifs_rx;
	ifs_tx.open ("tx_packets - 15 APR 2021.csv", std::ifstream::in);
	ifs_rx.open ("rx_packets - 15 APR 2021.csv", std::ifstream::in);

	std::vector<std::vector<std::string>> tx_table = readCSV(ifs_tx);
	std::vector<std::vector<std::string>> rx_table = readCSV(ifs_rx);
	
	// Compare tx_table and rx_table to generate results_table
	std::vector<result_elem> results_table = packet_compare(tx_table, rx_table);
	
	// Analyze results_table and calculate optimized parameters for d_o, K, gamma
	my_channel_model.optimize_model(results_table);
	
	std::cout << "Optimized model reference distance (d_o): " << my_channel_model.get_d_ref() << std::endl;
	std::cout << "Optimized model path loss @ d_o (K): " << my_channel_model.get_k_param() << std::endl;
	std::cout << "Optimized model gamma value: " << my_channel_model.get_gamma_param() << std::endl;
	std::cout << "Optimized model standard deviation from model: " << my_channel_model.get_std() << std::endl;

	// Generate scatter plot of PL vs distance, along with overlay
	// of Simplified Path Loss model generated earlier
	gen_scatterplot(results_table, my_channel_model);

	// Generate and print table of packet loss % binned by 50m distances
	std::vector<double> loss_table = gen_loss_table(results_table);

	//for (auto &elem : results_table) {
	//	std::cout << elem.packet_id <<"packet / "<< elem.rx_lat<<" "<< elem.rx_long <<" / "<< elem.tx_lat <<" "<<elem.tx_long<<" / "<<elem.distance_m<<"meters / "<<elem.successful<<" success / "<<elem.pathloss_measured_db<<" measured / "<<elem.pathloss_modeled_db<<"modeled / "<<elem.tx_power<<"tx power."<<std::endl;
	//}

	// Generate plot of packet loss % vs binned distance
	gen_lossplot(loss_table);

	ifs_tx.close();
	ifs_rx.close();

	return 0;
}

