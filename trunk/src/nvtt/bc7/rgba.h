/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef _RGBA_H
#define _RGBA_H

#define	RGBA_MIN	0
#define	RGBA_MAX	255		// range of RGBA

class RGBA
{
public:
	float r, g, b, a;
	RGBA(): r(0), g(0), b(0), a(0){}
	RGBA(float r, float g, float b, float a): r(r), g(g), b(b), a(a){}
};

#endif
