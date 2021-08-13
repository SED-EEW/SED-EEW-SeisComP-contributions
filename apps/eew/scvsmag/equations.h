/***************************************************************************
 *   Copyright (C) by ETHZ/SED                                             *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU Affero General Public License as published*
 * by the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU Affero General Public License for more details.                     *
 *                                                                         *
 * Authors of the Software: Michael Fischer and Yannik Behr                *
 ***************************************************************************/

#ifndef VSEQUATIONS_H_
#define VSEQUATIONS_H_

class VsEquations {
private:
	float mag, eqlat, eqlon, norm;
	// HA, HV, HD, ZA, ZV, ZD
	static const float attenuation[2][6][2][7];
public:
	VsEquations();
	virtual ~VsEquations();
	const float geta(int PSclass, int SoilClass, int WaveType);
	const float getb(int PSclass, int SoilClass, int WaveType);
	const float getc1(int PSclass, int SoilClass, int WaveType);
	const float getc2(int PSclass, int SoilClass, int WaveType);
	const float getd(int PSclass, int SoilClass, int WaveType);
	const float gete(int PSclass, int SoilClass, int WaveType);
	const float getsigma(int PSclass, int SoilClass, int WaveType);
	float ground_motion_ratio(float ZA, float ZD);
	int psclass(float ZA, float ZV, float HA, float HV);
	float mest(float ZAD, const int PSclass);
	float mesterr(const int PSclass);
	float zavg(const int PSclass);
	float zavgerr(const int PSclass);
	float saturation(const float c1, const float c2);
	float edist(const float stlat, const float stlon);
	float edistalt(const float stlat, const float stlon);
	float amplitude(const float a, const float b, const float c1,
			const float c2, const float d, const float e, const float stlat,
			const float stlon);
	float likelihood(float ZA, float ZV, float ZD, float HA, float HV, float HD,
			int PSclass, int Soilclass, float stlat, float stlon);
	// getters and setters
	const float geteqlat();
	void seteqlat(float eventlat);
	const float geteqlon();
	void seteqlon(float eventlon);
	const float getmag();
	void setmag(float magnitude);
	const float getnorm();
	void setnorm(float norminit);

};

#endif /* VSEQUATIONS_H_ */
