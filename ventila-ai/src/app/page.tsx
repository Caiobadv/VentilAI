"use client";

import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Card, CardHeader, CardContent } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Badge } from "@/components/ui/badge";
import { Copy, Send, Plus, Trash2, Radio } from "lucide-react";

const MAX_DEVICES = 5;

interface Device {
  id: number;
  name: string;
  type: string;
  status: "active" | "inactive";
  signal: string;
  hasSignal: boolean;
}

export default function Home() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [newDevice, setNewDevice] = useState<Partial<Device>>({
    name: "",
    type: "ar-condicionado",
    signal: "",
    hasSignal: false,
  });

  const addDevice = () => {
    if (devices.length >= MAX_DEVICES || !newDevice.name) return;
    
    const device: Device = {
      id: devices.length + 1,
      name: newDevice.name,
      type: newDevice.type || "ar-condicionado",
      status: "inactive",
      signal: "",
      hasSignal: false,
    };
    
    setDevices([...devices, device]);
    setNewDevice({
      name: "",
      type: "ar-condicionado",
      signal: "",
      hasSignal: false,
    });
  };

  const deleteDevice = (id: number) => {
    setDevices(devices.filter((device) => device.id !== id));
  };

  const copySignal = (signal: string) => {
    navigator.clipboard.writeText(signal);
  };

  const sendSignal = (signal: string) => {
    // Implement signal sending logic here
    console.log("Sending signal:", signal);
  };

  const receiveSignal = (id: number) => {
    // Simulate receiving a signal
    const newSignal = "0000 0000 0000 0000"; // This would be the actual received signal
    setDevices(
      devices.map((device) =>
        device.id === id
          ? { ...device, signal: newSignal, hasSignal: true }
          : device
      )
    );
  };

  const updateDevice = (id: number, field: keyof Device, value: string) => {
    setDevices(
      devices.map((device) =>
        device.id === id ? { ...device, [field]: value } : device
      )
    );
  };

  const updateNewDevice = (field: keyof Device, value: string) => {
    setNewDevice(prev => ({ ...prev, [field]: value }));
  };

  return (
    <main className="min-h-screen bg-background p-8">
      <div className="max-w-7xl mx-auto space-y-8">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-4xl font-bold text-foreground">VentilAI</h1>
            <p className="text-muted-foreground mt-2">
              Controle universal para seus dispositivos
            </p>
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {/* New Device Card */}
          <Card className="bg-card border-border border-dashed">
            <CardHeader className="space-y-4">
              <div className="flex items-center justify-between">
                <Input
                  value={newDevice.name}
                  onChange={(e) => updateNewDevice("name", e.target.value)}
                  placeholder="Nome do novo dispositivo"
                  className="text-lg font-semibold bg-background"
                />
              </div>
              <div className="flex items-center gap-2">
                <Select
                  value={newDevice.type}
                  onValueChange={(value) => updateNewDevice("type", value)}
                >
                  <SelectTrigger className="w-[180px] bg-background">
                    <SelectValue placeholder="Tipo do dispositivo" />
                  </SelectTrigger>
                  <SelectContent className="!bg-neutral-900 !text-white border-border">
                    <SelectItem value="ar-condicionado" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">Ar Condicionado</SelectItem>
                    <SelectItem value="ventilador" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">Ventilador</SelectItem>
                    <SelectItem value="tv" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">TV</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </CardHeader>
            <CardContent className="space-y-4">
              <Button
                onClick={addDevice}
                disabled={devices.length >= MAX_DEVICES || !newDevice.name}
                className="w-full bg-primary hover:bg-primary/90"
              >
                <Plus className="w-4 h-4 mr-2" />
                Adicionar Dispositivo
              </Button>
            </CardContent>
          </Card>

          {/* Existing Devices */}
          {devices.map((device) => (
            <Card key={device.id} className="bg-card border-border">
              <CardHeader className="space-y-4">
                <div className="flex items-center justify-between">
                  <Input
                    value={device.name}
                    onChange={(e) => updateDevice(device.id, "name", e.target.value)}
                    className="text-lg font-semibold bg-transparent border-none focus-visible:ring-0 focus-visible:ring-offset-0 p-0"
                  />
                  <Button
                    variant="ghost"
                    size="icon"
                    onClick={() => deleteDevice(device.id)}
                    className="text-muted-foreground hover:text-destructive"
                  >
                    <Trash2 className="w-4 h-4" />
                  </Button>
                </div>
                <div className="flex items-center gap-2">
                  <Select
                    value={device.type}
                    onValueChange={(value) => updateDevice(device.id, "type", value)}
                  >
                    <SelectTrigger className="w-[180px] bg-background">
                      <SelectValue placeholder="Tipo do dispositivo" />
                    </SelectTrigger>
                    <SelectContent className="!bg-neutral-900 !text-white border-border">
                      <SelectItem value="ar-condicionado" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">Ar Condicionado</SelectItem>
                      <SelectItem value="ventilador" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">Ventilador</SelectItem>
                      <SelectItem value="tv" className="!bg-neutral-900 !text-white hover:!bg-neutral-800">TV</SelectItem>
                    </SelectContent>
                  </Select>
                  <Badge
                    variant={device.hasSignal ? "default" : "secondary"}
                    className="ml-auto"
                  >
                    {device.hasSignal ? "Sinal Recebido" : "Sem Sinal"}
                  </Badge>
                </div>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex gap-2">
                  <Button
                    variant="outline"
                    className="flex-1 border-zinc-600 text-zinc-300 hover:bg-zinc-800"
                    onClick={() => receiveSignal(device.id)}
                  >
                    <Radio className="w-4 h-4 mr-2" />
                    Registrar Sinal
                  </Button>
                  <Button
                    onClick={() => sendSignal(device.signal)}
                    disabled={!device.hasSignal}
                    className="flex-1 bg-zinc-100 text-black font-bold shadow-lg border-0 hover:bg-zinc-200 dark:bg-zinc-100 dark:text-black dark:hover:bg-zinc-200"
                  >
                    <Send className="w-4 h-4 mr-2" />
                    Transmitir Sinal
                  </Button>
                </div>
              </CardContent>
            </Card>
          ))}
        </div>
      </div>
    </main>
  );
}
