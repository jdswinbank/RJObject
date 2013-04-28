#include <iostream>
#include <cmath>
#include <gsl/gsl_cdf.h>
#include "RandomNumberGenerator.h"
#include "Utils.h"
#include "Elliptical2D.h"

using namespace DNest3;
using namespace std;

Elliptical2D::Elliptical2D(double x_min, double x_max,
				double y_min, double y_max)
:x_min(x_min)
,x_max(x_max)
,y_min(y_min)
,y_max(y_max)
{
	if(x_min > x_max || y_min > y_max)
		cerr<<"# WARNING: Oddness in Elliptical2D constructor"<<endl;
	length_scale = sqrt((x_max - x_min)*
				(y_max - y_min));
}

void Elliptical2D::fromPrior()
{
	xc = x_min + (x_max - x_min)*randomU();
	yc = y_min + (y_max - y_min)*randomU();
	q = exp(log(0.2) + log(5./0.2)*randomU());
	theta = 2.*M_PI*randomU();
	cos_theta = cos(theta);
	sin_theta = sin(theta);

	mu_r = exp(log(0.01*length_scale) + log(100.)*randomU());
}

void Elliptical2D::print(ostream& out) const
{
	out<<xc<<' '<<yc<<' ';
	out<<q<<' '<<theta<<' ';
	out<<mu_r<<' ';
}

double Elliptical2D::perturb_parameters()
{
	double logH = 0.;

	double scale = pow(10., 1.5 - 6.*randomU());
	int which = randInt(4);
	if(which == 0)
	{
		xc += length_scale*scale*randn();
		yc += length_scale*scale*randn();
		xc = mod(xc - x_min, x_max - x_min) + x_min;
		yc = mod(yc - y_min, y_max - y_min) + y_min;
	}
	else if(which == 1)
	{
		q = log(q);
		q += log(5./0.2)*scale*randn();
		q = mod(q - log(0.2), log(5./0.2)) + log(0.2);
		q = exp(q);
	}
	else if(which == 2)
	{
		theta += 2.*M_PI*scale*randn();
		theta = mod(theta, 2.*M_PI);
		cos_theta = cos(theta);
		sin_theta = sin(theta);
	}
	else
	{
		mu_r = log(mu_r);
		mu_r += log(100.)*scale*randn();
		mu_r = mod(mu_r - log(0.01*length_scale),
				log(100.)) + log(0.01*length_scale);
		mu_r = exp(mu_r);
	}

	return logH;
}

double Elliptical2D::position_log_pdf(const vector<double>& position) const
{
	vector<double> vec = position;
	position_to_uniform(vec);

	double r = -mu_r*log(1. - vec[0]);
	double phi = 2.*M_PI*vec[1];

	double J11, J12, J21, J22;
	J11 = (1./sqrt(q)*cos_theta*cos(phi) - sqrt(q)*sin_theta*sin(phi))*
			(mu_r/(vec[0] - 1.));
	J12 = -2.*M_PI*r/sqrt(q)*cos_theta*sin(phi) -
		2.*M_PI*r*sqrt(q)*sin_theta*cos(phi);
	J21 = J12;
	J22 = -2.*M_PI*r/sqrt(q)*sin_theta*sin(phi) -
		2.*M_PI*r*sqrt(q)*cos_theta*cos(phi);

	vector<double> test1 = position;
	vector<double> test2 = position;
	test1[0] += 1E-7;
	test2[0] -= 1E-7;
	position_to_uniform(test1);
	position_to_uniform(test2);
	cout<<(test2[0] - test1[0])/2E-7<<' '<<1./J11<<endl;

	return 1./abs(J11*J22 - J12*J21);
}

void Elliptical2D::position_from_uniform(vector<double>& vec) const
{
	double r = -mu_r*log(1. - vec[0]);
	double phi = 2.*M_PI*vec[1];
	double xx = r*cos(phi)/sqrt(q);
	double yy = r*sin(phi)*sqrt(q);
	vec[0] = xc + cos_theta*xx - sin_theta*yy;
	vec[1] = yc + sin_theta*xx + cos_theta*yy;
}

void Elliptical2D::position_to_uniform(vector<double>& vec) const
{
	double xx, yy, r, phi;
	xx =  cos_theta*(vec[0] - xc) + sin_theta*(vec[1] - yc);
	yy = -sin_theta*(vec[0] - xc) + cos_theta*(vec[1] - yc);
	phi = atan2(yy, q*xx);
	if(phi < 0.)
		phi += 2.*M_PI;
	r = xx*sqrt(q)/cos(phi);
	vec[0] = 1. - exp(-r/mu_r);
	vec[1] = phi/(2.*M_PI);
}
