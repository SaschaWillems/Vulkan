// Copyright 2020 Google LLC

[shader("miss")]
void main([[vk::location(2)]] in bool shadowed)
{
	shadowed = false;
}