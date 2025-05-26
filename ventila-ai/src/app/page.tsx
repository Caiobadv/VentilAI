"use client";

import { useState, useEffect } from "react";
import { Button } from "@/components/ui/button";
import { Card, CardHeader, CardContent } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Send, Plus, Trash2, Radio, Pencil, Check, X, ScrollText } from "lucide-react";
import { db } from "@/firebase";
import {
  ref,
  onValue,
  set,
  push,
  update,
  remove,
  off,
} from "firebase/database";
import Link from "next/link";

const MAX_DEVICES = 5;

interface Device {
  id: string;
  name: string;
  description?: string;
  mock?: boolean;
  ir_code: string;
  code_size: number;
  empty: boolean;
}

export default function Home() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [newDevice, setNewDevice] = useState<Partial<Device>>({
    name: "",
    description: "",
    ir_code: "",
    code_size: 0,
    empty: true,
  });
  const [loading, setLoading] = useState(true);
  const [waitingForSignal, setWaitingForSignal] = useState<string | null>(null);
  const [transmitting, setTransmitting] = useState<string | null>(null);
  const [isTransmitting, setIsTransmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [showLoadingPopup, setShowLoadingPopup] = useState(false);
  const [editing, setEditing] = useState<{ [id: string]: { name: boolean; description: boolean } }>({});
  const [editBuffer, setEditBuffer] = useState<{ [id: string]: { name: string; description: string } }>({});
  const [showNameWarning, setShowNameWarning] = useState(false);
  const [showDuplicateNameWarning, setShowDuplicateNameWarning] = useState(false);

  function toDevice(id: string, value: any): Device {
    return {
      id: id as string,
      name: value.name,
      description: value.description,
      ir_code: value.ir_code,
      code_size: value.code_size,
      empty: value.empty,
    };
  }

  // DELETAR
  useEffect(() => {
    setDevices((prev) => {
      if (prev.some((d) => d.mock)) return prev;
      return [
        {
          id: "mock-device-1",
          name: "Controle TV Sala (Mock)",
          description: "TV - Mocked Device (delete me)",
          mock: true,
          ir_code: "",
          code_size: 0,
          empty: true,
        },
        ...prev,
      ];
    });
  }, []);

  useEffect(() => {
    const devicesRef = ref(db, "devices");
    const unsubscribe = onValue(
      devicesRef,
      (snapshot) => {
        const data = snapshot.val();
        if (data) {
          const loaded: Device[] = Object.entries(data).map(([id, value]: any) => toDevice(id, value));
          // AJUSTAR LOGICA (APAGAR O MOCK)
          setDevices((prev) => {
            const mock = prev.find((d) => d.mock);
            return mock ? [mock, ...loaded] : loaded;
          });
        } else {
          setDevices((prev) => prev.filter((d) => d.mock));
        }
        setLoading(false);
      },
      (error) => {
        setLoading(false);
        setError("Erro ao carregar dispositivos.");
      }
    );
    return () => off(devicesRef, "value", unsubscribe);
  }, []);

  const addDevice = async () => {
    if (devices.length >= MAX_DEVICES) return;
    if (!newDevice.name) {
      setShowNameWarning(true);
      setShowDuplicateNameWarning(false);
      return;
    }
    const newName = newDevice.name.trim().toLowerCase();
    if (devices.some(d => d.name.trim().toLowerCase() === newName)) {
      setShowDuplicateNameWarning(true);
      setShowNameWarning(false);
      return;
    }
    setShowNameWarning(false);
    setShowDuplicateNameWarning(false);
    const deviceData = {
      name: newDevice.name,
      description: newDevice.description || "",
      ir_code: newDevice.ir_code || "",
      code_size: newDevice.code_size || 0,
      empty: newDevice.empty || true,
    };
    try {
      await push(ref(db, "devices"), deviceData);
    } catch (e) {
      const localId = typeof crypto !== 'undefined' && crypto.randomUUID ? crypto.randomUUID() : `local-${Date.now()}-${Math.floor(Math.random() * 10000)}`;
      setDevices([
        ...devices,
        { id: localId, ...deviceData },
      ]);
      setError("Erro ao adicionar dispositivo.");
    }
    setNewDevice({
      name: "",
      description: "",
      ir_code: "",
      code_size: 0,
      empty: true,
    });
  };

  const deleteDevice = async (id: string) => {
    // AJUSTAR LOGICA (APAGAR O MOCK)
    if (id.startsWith("mock-device")) {
      setDevices((prev) => prev.filter((d) => d.id !== id));
      return;
    }
    try {
      await remove(ref(db, `devices/${id}`));
    } catch (e) {
      setDevices(devices.filter((device) => device.id !== id));
      setError("Erro ao remover dispositivo.");
    }
  };

  const updateDevice = async (id: string, field: keyof Device, value: any) => {
    if (id.startsWith("mock-device")) {
      setDevices((prev) =>
        prev.map((device) =>
          device.id === id ? { ...device, [field]: value } : device
        )
      );
      return;
    }
    try {
      await update(ref(db, `devices/${id}`), { [field]: value });
    } catch (e) {
      setDevices(
        devices.map((device) =>
          device.id === id ? { ...device, [field]: value } : device
        )
      );
      setError("Erro ao atualizar dispositivo.");
    }
  };

  const requestSignalCapture = async (id: string) => {
    setWaitingForSignal(id);
    setShowLoadingPopup(true);
    setError(null);
    try {
      if (!id.startsWith("mock-device")) {
        await set(ref(db, `captureRequests/${id}`), true);
        const emptyRef = ref(db, `devices/${id}/empty`);
        const unsubscribe = onValue(emptyRef, (snapshot) => {
          if (snapshot.exists() && snapshot.val() === false) {
            setWaitingForSignal(null);
            setShowLoadingPopup(false);
            off(emptyRef);
          }
        });
      } else {
        // AJUSTAR LOGICA (APAGAR O MOCK)
        setTimeout(() => {
          setWaitingForSignal(null);
          setShowLoadingPopup(false);
        }, 1500);
      }
    } catch (e) {
      setWaitingForSignal(null);
      setShowLoadingPopup(false);
      setError("Erro ao solicitar captura de sinal.");
    }
  };

  const requestTransmit = async (id: string, signal: any) => {
    setTransmitting(id);
    setIsTransmitting(true);
    setShowLoadingPopup(true);
    setError(null);
    try {
      if (!id.startsWith("mock-device")) {
        await set(ref(db, `transmitRequests/${id}`), true);
        
        const transmitRef = ref(db, `transmitRequests/${id}`);
        const unsubscribe = onValue(transmitRef, (snapshot) => {
          if (snapshot.exists() && snapshot.val() === false) {
            setTransmitting(null);
            setIsTransmitting(false);
            setShowLoadingPopup(false);
            off(transmitRef);
          }
        });

      } else {
        // AJUSTAR LOGICA (APAGAR O MOCK)
        setTimeout(() => {
          setTransmitting(null);
          setIsTransmitting(false);
          setShowLoadingPopup(false);
        }, 1500);
      }
    } catch (e) {
      setTransmitting(null);
      setIsTransmitting(false);
      setShowLoadingPopup(false);
      setError("Erro ao solicitar transmissão de sinal.");
    }
  };

  const updateNewDevice = (field: keyof Device, value: any) => {
    setNewDevice((prev) => ({ ...prev, [field]: value }));
  };

  const startEdit = (id: string, field: "name" | "description", value: string) => {
    setEditing((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: true },
    }));
    setEditBuffer((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: value },
    }));
  };

  const cancelEdit = (id: string, field: "name" | "description") => {
    setEditing((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: false },
    }));
    setEditBuffer((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: "" },
    }));
  };

  const saveEdit = async (id: string, field: "name" | "description") => {
    const value = editBuffer[id]?.[field] || "";
    await updateDevice(id, field, value);
    setEditing((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: false },
    }));
    setEditBuffer((prev) => ({
      ...prev,
      [id]: { ...prev[id], [field]: "" },
    }));
  };

  return (
		<main className="min-h-screen bg-background p-8">
			<div className="max-w-7xl mx-auto space-y-8">
				<div className="flex items-center justify-between">
					<div>
						<h1 className="text-4xl font-bold text-foreground">UbiRemote</h1>
						<p className="text-muted-foreground mt-2">
							Controle universal para seus dispositivos
						</p>
					</div>
					<Link href="/logs">
						<Button variant="outline" size="sm">
							<ScrollText className="mr-2 h-4 w-4" />
							Ver Logs
						</Button>
					</Link>
				</div>
				{error && (
					<div className="text-red-500 font-semibold mb-4">{error}</div>
				)}
				{showLoadingPopup && (
					<div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60">
						<div className="bg-zinc-900 text-white rounded-lg p-8 shadow-xl flex flex-col items-center gap-4 min-w-[300px]">
							<div className="animate-spin rounded-full h-10 w-10 border-b-2 border-white mb-2" />
							<div className="text-lg font-semibold text-center">
								{waitingForSignal
									? "Aguardando captura do sinal..."
									: isTransmitting
									? "Transmitindo sinal..."
									: "Processando..."}
							</div>
							<div className="text-sm text-zinc-400 text-center">
								{waitingForSignal
									? "Aponte o controle remoto para o sensor IR e pressione o botão desejado"
									: isTransmitting
									? "O sinal está sendo transmitido para o dispositivo"
									: "Aguarde enquanto processamos sua solicitação"}
							</div>
							<Button
								variant="destructive"
								className="mt-2 hover:text-red-500 cursor-pointer"
								onClick={async () => {
									if (waitingForSignal) {
										await set(
											ref(db, `captureRequests/${waitingForSignal}`),
											false
										);
									} else if (transmitting) {
										await set(
											ref(db, `transmitRequests/${transmitting}`),
											false
										);
									}
									setWaitingForSignal(null);
									setTransmitting(null);
									setIsTransmitting(false);
									setShowLoadingPopup(false);
								}}
							>
								Cancelar
							</Button>
						</div>
					</div>
				)}
				<div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
					{/* New Device Card */}
					<Card className="bg-card border-border border-dashed">
						<CardHeader className="space-y-4">
							<div className="flex flex-col gap-2 w-full max-w-sm">
								<label
									className="text-sm font-semibold text-foreground"
									htmlFor="new-device-name"
								>
									Nome do novo dispositivo{" "}
									<span className="text-yellow-400">*</span>
								</label>
								<Input
									id="new-device-name"
									value={newDevice.name}
									onChange={(e) => updateNewDevice("name", e.target.value)}
									placeholder="Ex: TV Sala, Ventilador Quarto..."
									className="text-lg font-semibold bg-background cursor-text"
									autoComplete="off"
								/>
								<label
									className="text-sm font-semibold text-foreground"
									htmlFor="new-device-desc"
								>
									Descrição/Categoria (opcional)
								</label>
								<Input
									id="new-device-desc"
									value={newDevice.description}
									onChange={(e) =>
										updateNewDevice("description", e.target.value)
									}
									placeholder="Ex: TV, Ar-condicionado, etc."
									className="bg-background cursor-text"
									autoComplete="off"
								/>
							</div>
						</CardHeader>
						<CardContent className="space-y-4 w-full max-w-sm">
							<div className="flex justify-center w-full">
								<Button
									onClick={addDevice}
									disabled={devices.length >= MAX_DEVICES || !newDevice.name}
									className="w-full bg-white text-black font-bold shadow border border-zinc-300 hover:bg-zinc-100 disabled:opacity-60 disabled:cursor-not-allowed transition-all px-6 py-3 cursor-pointer"
								>
									<Plus className="w-4 h-4 mr-2" />
									Criar Dispositivo
								</Button>
							</div>
							{devices.length >= MAX_DEVICES && (
								<div className="text-xs text-red-400 font-semibold text-center">
									Limite máximo de dispositivos atingido.
								</div>
							)}
							{showNameWarning && (
								<div className="text-xs text-yellow-400 font-semibold text-center">
									O nome do dispositivo é obrigatório.
								</div>
							)}
							{showDuplicateNameWarning && (
								<div className="text-xs text-yellow-400 font-semibold text-center">
									Já existe um dispositivo com esse nome.
								</div>
							)}
						</CardContent>
					</Card>
					{/* Existing Devices */}
					{devices.map((device) => (
						<Card
							key={device.id}
							className="bg-card border-border w-full max-w-full"
						>
							<CardHeader className="space-y-4">
								<div className="flex flex-col gap-2 w-full max-w-sm mx-auto">
									<div className="flex items-center gap-2 w-full">
										{editing[device.id]?.name ? (
											<>
												<Input
													value={editBuffer[device.id]?.name ?? device.name}
													onChange={(e) =>
														setEditBuffer((prev) => ({
															...prev,
															[device.id]: {
																...prev[device.id],
																name: e.target.value,
															},
														}))
													}
													className="text-lg font-semibold bg-background flex-1"
													autoFocus
												/>
												<Button
													size="icon"
													variant="ghost"
													className="text-green-500 cursor-pointer"
													onClick={() => saveEdit(device.id, "name")}
												>
													<Check className="w-4 h-4" />
												</Button>
												<Button
													size="icon"
													variant="ghost"
													className="text-red-500 cursor-pointer"
													onClick={() => cancelEdit(device.id, "name")}
												>
													<X className="w-4 h-4" />
												</Button>
											</>
										) : (
											<>
												<span className="text-lg font-semibold flex-1">
													{device.name}
												</span>
												<Button
													size="icon"
													variant="ghost"
													className="text-zinc-400 hover:text-primary cursor-pointer"
													onClick={() =>
														startEdit(device.id, "name", device.name)
													}
												>
													<Pencil className="w-4 h-4" />
												</Button>
											</>
										)}
									</div>
									<div className="flex items-center gap-2 w-full">
										{editing[device.id]?.description ? (
											<>
												<Input
													value={
														(editBuffer[device.id]?.description ??
															device.description) ||
														""
													}
													onChange={(e) =>
														setEditBuffer((prev) => ({
															...prev,
															[device.id]: {
																...prev[device.id],
																description: e.target.value,
															},
														}))
													}
													className="bg-background flex-1"
													autoFocus
												/>
												<Button
													size="icon"
													variant="ghost"
													className="text-green-500 cursor-pointer"
													onClick={() => saveEdit(device.id, "description")}
												>
													<Check className="w-4 h-4" />
												</Button>
												<Button
													size="icon"
													variant="ghost"
													className="text-red-500 cursor-pointer"
													onClick={() => cancelEdit(device.id, "description")}
												>
													<X className="w-4 h-4" />
												</Button>
											</>
										) : (
											<>
												<span className="text-zinc-400 flex-1">
													{device.description || (
														<span className="italic">Sem descrição</span>
													)}
												</span>
												<Button
													size="icon"
													variant="ghost"
													className="text-zinc-400 hover:text-primary cursor-pointer"
													onClick={() =>
														startEdit(
															device.id,
															"description",
															device.description || ""
														)
													}
												>
													<Pencil className="w-4 h-4" />
												</Button>
											</>
										)}
									</div>
									{device.mock && (
										<div className="text-xs text-yellow-400 font-semibold">
											Mocked device (delete me)
										</div>
									)}
								</div>
							</CardHeader>
							<CardContent className="space-y-4 w-full max-w-sm mx-auto">
								<div className="flex flex-col items-center gap-2 w-full">
									<div className="w-full flex justify-center">
										<Button
											className="w-full bg-white text-black font-bold shadow border border-zinc-300 hover:bg-zinc-100 cursor-pointer px-6 py-3"
											onClick={() => requestSignalCapture(device.id)}
											disabled={
												waitingForSignal === device.id || transmitting !== null
											}
										>
											<Radio className="w-4 h-4 mr-2" />
											<span>
												{waitingForSignal === device.id
													? "Aguardando Sinal..."
													: "Registrar Sinal"}
											</span>
										</Button>
									</div>

									<div className="w-full flex justify-center">
										<Button
											onClick={() => requestTransmit(device.id, device.ir_code)}
											disabled={
												transmitting === device.id ||
												waitingForSignal !== null ||
												device.empty
											}
											className="w-full bg-yellow-400 text-black font-bold shadow-lg border-0 hover:bg-yellow-300 cursor-pointer px-6 py-3"
										>
											<Send className="w-4 h-4 mr-2" />
											<span>
												{transmitting === device.id
													? "Transmitindo..."
													: "Transmitir Sinal"}
											</span>
										</Button>
									</div>
								</div>

								<div className="flex justify-center w-full">
									<Button
										variant="ghost"
										size="icon"
										onClick={() => deleteDevice(device.id)}
										className="text-zinc-400 hover:text-red-500 cursor-pointer"
									>
										<Trash2 className="w-4 h-4" />
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
